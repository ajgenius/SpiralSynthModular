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
#include "AtomicClock.h"
#include <algorithm>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
m_NextRetry(0),
m_Frame(0),
m_Rolling(true),
m_ThreadRunning(false),
m_ThreadStop(false),
m_Clock(NULL)
{
}

void AudioTransportHub::SetHost(const HostInfo *host)
{
	m_Host = host;
	OUTPUTCLIENT::host = host;
	delete m_Clock;
	m_Clock = NULL;
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
		StopTransportThread();
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
	m_IOFailed=false;
	m_NextRetry=time(NULL)+1;
	m_Mode=CLOSED;
	StopTransportThread();
	if (m_Configured && !m_Members.empty()) {
		OUTPUTCLIENT::Get()->SetCallback(TransportCallback, this);
		if (mode==INPUT) opened=OUTPUTCLIENT::Get()->OpenRead();

		if (mode==OUTPUT) opened=OUTPUTCLIENT::Get()->OpenWrite();
		if (mode==DUPLEX) opened=OUTPUTCLIENT::Get()->OpenReadWrite();
	}
	if (opened) m_Mode=mode;
	// A blocking device is fed from our own thread so it paces nothing
	// but itself; a callback device feeds itself.
	if (opened && !IsCallbackDriven()) StartTransportThread();
	ReportMode();
}

void AudioTransportHub::Close()
{
	m_RequestedMode=CLOSED;

	StopTransportThread();
	OUTPUTCLIENT::Get()->Close();
	m_Mode=CLOSED;
	ReportMode();
}

void AudioTransportHub::Reconfigure()
{
	const Mode previous=m_Mode;
	StopTransportThread();
	m_Configured=m_Host && OUTPUTCLIENT::Get()->Configure(
		m_Host->AUDIOCLIENT,
		m_Host->OUTPUTFILE);
	OUTPUTCLIENT::Get()->AllocateBuffer();
	OpenMode(previous==NO_MODE ? OUTPUT : previous);
}

// * Transport side

void AudioTransportHub::TransportCallback(void *context, unsigned int frames)
{
	if (frames) static_cast<AudioTransportHub *>(context)->TransportCycle();
}

void AudioTransportHub::TransportCycle()
{
	const bool read=m_Mode==INPUT || m_Mode==DUPLEX;
	const bool write=m_Mode==OUTPUT || m_Mode==DUPLEX;
	if (!OUTPUTCLIENT::Get()->TransportCycle(read, write)) m_IOFailed=true;
}

void *AudioTransportHub::TransportThread(void *context)
{
	AudioTransportHub *hub=static_cast<AudioTransportHub *>(context);
	const unsigned period=hub->PeriodMicroseconds();
	while (!hub->m_ThreadStop)
	{
		if (!OUTPUTCLIENT::Get()->WaitReady(period*4)) continue;
		hub->TransportCycle();
		if (hub->m_IOFailed) break;
	}
	return NULL;
}

void AudioTransportHub::StartTransportThread()
{
	if (m_ThreadRunning) return;
	m_ThreadStop=false;
	m_ThreadRunning=true;
	if (pthread_create(&m_Thread, NULL, TransportThread, this)) m_ThreadRunning=false;
}

void AudioTransportHub::StopTransportThread()
{
	// m_ThreadRunning means created and not yet joined: a thread that left
	// on an I/O failure is still joined here.
	if (!m_ThreadRunning) return;
	// A blocking Write returns within one period and WaitReady within a
	// few, so the thread leaves on its own before the device closes.
	m_ThreadStop=true;
	pthread_join(m_Thread, NULL);
	m_ThreadRunning=false;
	m_ThreadStop=false;
}

unsigned AudioTransportHub::PeriodMicroseconds() const
{
	if (!m_Host || m_Host->SAMPLERATE<=0 || m_Host->BUFSIZE<=0) return 10000;
	return (unsigned)((unsigned long long)m_Host->BUFSIZE*1000000ULL/m_Host->SAMPLERATE);
}

// * Engine side

bool AudioTransportHub::Streaming() const
{
	return m_Mode!=CLOSED && m_Mode!=NO_MODE && OUTPUTCLIENT::Get()->IsAttached();
}

bool AudioTransportHub::WaitPeriod()
{
	const unsigned period=PeriodMicroseconds();
	if (Streaming())
	{
		if (m_Clock) { delete m_Clock; m_Clock=NULL; }
		return OUTPUTCLIENT::Get()->WaitPeriod(period*4);
	}

	// No stream: the platform clock keeps the engine at the same rate.
#ifdef HAVE_ATOMIC_CLOCK
	// Derive the clock from frames/rate directly; truncating a period to
	// whole microseconds introduces a persistent frequency error.
	const float frequency = m_Host && m_Host->BUFSIZE > 0 && m_Host->SAMPLERATE > 0
		? float(m_Host->SAMPLERATE) / m_Host->BUFSIZE : 100.f;
	if (m_Clock && m_Clock->Frequency()!=frequency) { delete m_Clock; m_Clock=NULL; }
	if (!m_Clock) m_Clock=new AtomicClock(frequency);
	m_Clock->Tick();
#else
	usleep(period);
#endif
	return true;
}

void AudioTransportHub::BeginPeriod()
{
	// The stream's transport, when it has one, says where this period is.
	unsigned long frame; bool rolling;
	if (Streaming() && OUTPUTCLIENT::Get()->GetTransport(frame, rolling)) { m_Frame=frame; m_Rolling=rolling; }
}

void AudioTransportHub::CommitPeriod()
{
	if (m_Mode!=CLOSED && m_Mode!=NO_MODE) OUTPUTCLIENT::Get()->CommitPeriod();
	if (m_Rolling && m_Host) m_Frame+=m_Host->BUFSIZE;
}

void AudioTransportHub::Start()
{
	if (!(Streaming() && OUTPUTCLIENT::Get()->StartTransport())) m_Rolling=true;
}

void AudioTransportHub::Stop()
{
	if (!(Streaming() && OUTPUTCLIENT::Get()->StopTransport())) m_Rolling=false;
}

void AudioTransportHub::Locate(unsigned long frame)
{
	if (!(Streaming() && OUTPUTCLIENT::Get()->LocateTransport(frame))) m_Frame=frame;
}

void AudioTransportHub::Service()
{
	if (m_Members.empty()) return;
	if (m_Mode==NO_MODE) OpenMode(OUTPUT);

	if (m_IOFailed)
	{
		StopTransportThread();
		OUTPUTCLIENT::Get()->Close();
		m_Mode=CLOSED;
		ReportMode();

		m_IOFailed=false;
	}

	// Server shutdown removes the native ports without another audio callback.
	// Keep the requested mode, but reopen only here on the control thread.
	if (IsCallbackDriven() && !OUTPUTCLIENT::Get()->IsAttached() &&
		m_RequestedMode!=CLOSED && m_RequestedMode!=NO_MODE)
	{
		if (m_Mode!=CLOSED)
		{
			StopTransportThread();
			OUTPUTCLIENT::Get()->Close();
			m_Mode=CLOSED;
			ReportMode();
		}

		if (time(NULL)>=m_NextRetry) OpenMode(m_RequestedMode);

	}

	if (IsCallbackDriven() && OUTPUTCLIENT::Get()->IsAttached() && OUTPUTCLIENT::Get()->BufferSize())
		m_Members.front()->NotifyBufferAndSampleRate(OUTPUTCLIENT::Get()->BufferSize(), OUTPUTCLIENT::Get()->SampleRate());

}

