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
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

using namespace std;
using namespace spiralcore;

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
m_NextRetry(0)
{
}

void AudioTransportHub::SetHost(const HostInfo *host)
{
	m_Host = host;
	OUTPUTCLIENT::host = host;
	m_Timeline.Reset();
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

	m_Members.erase(std::remove(m_Members.begin(),m_Members.end(),endpoint),m_Members.end());
	if (m_Members.empty()) {
		OUTPUTCLIENT::PackUpAndGoHome();
		m_Mode=NO_MODE;
		m_Configured=false;
	}
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
	m_NextRetry=time(NULL)+1;
	m_Mode=CLOSED;
	if (m_Configured && !m_Members.empty()) {
		if (mode==INPUT) opened=OUTPUTCLIENT::Get()->OpenRead();

		if (mode==OUTPUT) opened=OUTPUTCLIENT::Get()->OpenWrite();
		if (mode==DUPLEX) opened=OUTPUTCLIENT::Get()->OpenReadWrite();
	}
	if (opened) m_Mode=mode;
	ReportMode();
}

void AudioTransportHub::Close()
{
	m_RequestedMode=CLOSED;

	OUTPUTCLIENT::Get()->Close();
	m_Mode=CLOSED;
	ReportMode();
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

void AudioTransportHub::RegisterStream(AudioStream *stream)
{
	m_Timeline.Register(stream);
}

void AudioTransportHub::UnregisterStream(AudioStream *stream)
{
	m_Timeline.Unregister(stream);
}

bool AudioTransportHub::Streaming() const
{
	return m_Mode != CLOSED && m_Mode != NO_MODE && OUTPUTCLIENT::Get()->IsAttached();
}

bool AudioTransportHub::PreparePeriod()
{
	return PreparePeriod(AudioMonotonicTime());
}

bool AudioTransportHub::PreparePeriod(double now)
{
	return m_Host && m_Timeline.Prepare(m_Host->BUFSIZE, m_Host->SAMPLERATE, now,
		Streaming() ? OUTPUTCLIENT::Get()->Stream() : NULL);
}

unsigned AudioTransportHub::SleepMicroseconds() const
{
	return m_Timeline.SleepMicroseconds();
}

bool AudioTransportHub::WaitPeriod()
{
	while (!PreparePeriod())
	{
		if (!m_Host) return false;

		usleep(SleepMicroseconds());
	}

	return true;
}

void AudioTransportHub::BeginPeriod()
{
	m_Timeline.Begin();
	if (Streaming()) OUTPUTCLIENT::Get()->BeginPeriod(m_Timeline.CaptureStamp());
}

void AudioTransportHub::CommitPeriod()
{
	if (Streaming()) OUTPUTCLIENT::Get()->CommitPeriod(m_Timeline.PlaybackStamp());

	if (m_Host) m_Timeline.Commit(m_Host->BUFSIZE);
}

void AudioTransportHub::Start() { m_Timeline.Start(); }
void AudioTransportHub::Stop() { m_Timeline.Stop(); }
void AudioTransportHub::Locate(unsigned long frame) { m_Timeline.Locate(frame); }

void AudioTransportHub::Service()
{
	if (m_Members.empty()) return;
	if (m_Mode==NO_MODE) OpenMode(OUTPUT);

	// Server shutdown removes the native ports without another audio callback.
	// Keep the requested mode, but reopen only here on the control thread.
	if ((!OUTPUTCLIENT::Get()->IsAttached() || OUTPUTCLIENT::Get()->Stream()->Failed()) &&
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

