/*  SpiralSound
 *  Copyleft (C) 2001 David Griffiths <dave@pawfal.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "AudioTransportHub.h"
#include <algorithm>

using namespace std;

AudioTransportHub *AudioTransportHub::m_Singleton = NULL;

AudioTransportHub *AudioTransportHub::Get()
{
	if (!m_Singleton) m_Singleton = new AudioTransportHub;
	return m_Singleton;
}

AudioTransportHub::AudioTransportHub() :
m_Host(NULL),
m_Configured(false),
m_Mode(NO_MODE),
m_RequestedMode(OUTPUT),
m_IOFailed(false),
m_NextRetry(0)
{
}

void AudioTransportHub::Attach(AudioEndpoint *endpoint, const HostInfo *host)
{
	if (find(m_Members.begin(),m_Members.end(),endpoint)==m_Members.end())
		m_Members.push_back(endpoint);

	m_Host=host;
	OUTPUTCLIENT::host = host;
	if (!m_Configured) {
		m_Configured = OUTPUTCLIENT::Get()->Configure(host->AUDIOCLIENT, host->OUTPUTFILE);
		if (!m_Configured) m_Mode=CLOSED;
	}
	OUTPUTCLIENT::Get()->AllocateBuffer();
}

void AudioTransportHub::Detach(AudioEndpoint *endpoint)
{
	if (find(m_Members.begin(),m_Members.end(),endpoint)==m_Members.end()) return;

	// Replace the callback context only after its client has stopped.
	if (IsRepresentative(endpoint) && IsCallbackDriven())
	{
		OUTPUTCLIENT::Get()->Close();
		if (m_Mode != CLOSED) m_Mode = NO_MODE;
	}

	m_Members.erase(std::remove(m_Members.begin(),m_Members.end(),endpoint),m_Members.end());
	if (m_Members.empty()) {
		Blocking(endpoint,false);
		OUTPUTCLIENT::PackUpAndGoHome();
		m_Mode=NO_MODE;
		m_Configured=false;
	}
}

void AudioTransportHub::Blocking(AudioEndpoint *endpoint, bool mode)
{
	if (!m_Members.empty()) endpoint=m_Members.front();
	if (endpoint) endpoint->Blocking(mode);
}

void AudioTransportHub::ReportMode()
{
	for (size_t i=0;i<m_Members.size();++i)
		m_Members[i]->TransportModeChanged((int)m_Mode);
}

void AudioTransportHub::OpenMode(Mode mode)
{
	m_RequestedMode=mode;

	bool opened=false;
	m_IOFailed=false;
	m_NextRetry=time(NULL)+1;
	m_Mode=CLOSED;
	if (m_Configured && !m_Members.empty()) {
		OUTPUTCLIENT::Get()->SetCallback(ProcessCallback, m_Members.front());
		if (mode==INPUT) opened=OUTPUTCLIENT::Get()->OpenRead();

		if (mode==OUTPUT) opened=OUTPUTCLIENT::Get()->OpenWrite();
		if (mode==DUPLEX) opened=OUTPUTCLIENT::Get()->OpenReadWrite();
	}
	if (opened) m_Mode=mode;
	ReportMode();
	Blocking(NULL,opened && !IsCallbackDriven());
}

void AudioTransportHub::Close()
{
	m_RequestedMode=CLOSED;

	OUTPUTCLIENT::Get()->Close();
	m_Mode=CLOSED;
	ReportMode();
	Blocking(NULL,false);
}

void AudioTransportHub::Reconfigure()
{
	const Mode previous=m_Mode;
	m_Configured=m_Host && OUTPUTCLIENT::Get()->Configure(
		m_Host->AUDIOCLIENT,
		m_Host->OUTPUTFILE);
	OUTPUTCLIENT::Get()->AllocateBuffer();
	OpenMode(previous==NO_MODE ? OUTPUT : previous);
}

void AudioTransportHub::ProcessCallback(void *context, unsigned int frames)
{
	static_cast<AudioEndpoint *>(context)->RunAudioCycle(frames);
}

void AudioTransportHub::Service()
{
	if (m_Members.empty()) return;
	if (m_Mode==NO_MODE) OpenMode(OUTPUT);

	if (m_IOFailed)
	{
		OUTPUTCLIENT::Get()->Close();
		m_Mode=CLOSED;
		ReportMode();
		Blocking(NULL,false);

		m_IOFailed=false;
	}

	// Server shutdown removes the native ports without another audio callback.
	// Keep the requested mode, but reopen only here on the control thread.
	if (IsCallbackDriven() && !OUTPUTCLIENT::Get()->IsAttached() &&
		m_RequestedMode!=CLOSED && m_RequestedMode!=NO_MODE)
	{
		if (m_Mode!=CLOSED)
		{
			OUTPUTCLIENT::Get()->Close();
			m_Mode=CLOSED;
			ReportMode();
		}

		if (time(NULL)>=m_NextRetry) OpenMode(m_RequestedMode);

	}

	if (IsCallbackDriven() && OUTPUTCLIENT::Get()->IsAttached() && OUTPUTCLIENT::Get()->BufferSize())
		m_Members.front()->NotifyBufferAndSampleRate(OUTPUTCLIENT::Get()->BufferSize(), OUTPUTCLIENT::Get()->SampleRate());

}

void AudioTransportHub::Process()
{
	if (m_Members.empty()) return;

	bool ok=true;
	if (m_Mode==INPUT || m_Mode==DUPLEX) ok=OUTPUTCLIENT::Get()->Read();

	if (ok && (m_Mode==OUTPUT || m_Mode==DUPLEX)) ok=OUTPUTCLIENT::Get()->Play();

	m_IOFailed = !ok;
}
