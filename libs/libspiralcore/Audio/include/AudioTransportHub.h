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

#ifndef AUDIO_TRANSPORT_HUB
#define AUDIO_TRANSPORT_HUB

#include "AudioDriver.h"
#include "OutputAudioClient.h"
#include <vector>
#include <ctime>
#include "AudioTimeline.h"

class AtomicClock;

class AudioEndpoint : public AudioDriver
{
public:
	virtual void TransportModeChanged(int mode) {}

	void NotifyBufferAndSampleRate(unsigned long frames, unsigned long rate)
		{ if (ChangeBufferAndSampleRate) ChangeBufferAndSampleRate(frames, rate, m_Parent); }
};

class AudioTransportHub
{
public:
	enum Mode {NO_MODE,INPUT,OUTPUT,DUPLEX,CLOSED};

	static AudioTransportHub *Get();

	void SetHost(const HostInfo *host);
	void BeginPatchReplacement();
	void EndPatchReplacement();

	void Attach(AudioEndpoint *endpoint, const HostInfo *host);
	void Detach(AudioEndpoint *endpoint);
	bool IsRepresentative(const AudioEndpoint *endpoint) const
		{ return !m_Members.empty() && m_Members.front()==endpoint; }

	OutputAudioClient *Transport() const { return OUTPUTCLIENT::Get(); }
	bool IsCallbackDriven() const { return Transport()->IsCallbackDriven(); }
	Mode GetMode() const { return m_Mode; }
	unsigned Underruns() const { return Transport()->Underruns(); }

	void OpenMode(Mode mode);
	void Close();
	void Reconfigure();

	void Service();
	// Control-thread maintenance. Opening JACK can block while the server is down.
	static void ServiceConnections();

	void RegisterStream(Spiral::Core::AudioStream *stream);
	void UnregisterStream(Spiral::Core::AudioStream *stream);
	bool PreparePeriod();
	bool PreparePeriod(double monotonicNow);
	unsigned SleepMicroseconds() const;
	bool WaitPeriod();
	void BeginPeriod();
	void CommitPeriod();
	const Spiral::Core::AudioStamp &PlaybackStamp() const { return m_Timeline.PlaybackStamp(); }
	const Spiral::Core::AudioStamp &CaptureStamp() const { return m_Timeline.CaptureStamp(); }

	unsigned long Frame() const { return m_Timeline.Frame(); }
	bool Rolling() const { return m_Timeline.Rolling(); }
	void Start();
	void Stop();
	void Locate(unsigned long frame);

private:
	AudioTransportHub();
	void ReportMode();

	static AudioTransportHub *m_Singleton;
	std::vector<AudioEndpoint *> m_Members;
	const HostInfo *m_Host;
	void ReleaseTransport();
	bool m_Configured;
	bool m_Replacing;
	std::string m_ReplacementDriver;
	int m_ReplacementRate, m_ReplacementFrames, m_ReplacementFragmentSize, m_ReplacementFragmentCount;
	Mode m_Mode;
	Mode m_RequestedMode;
	time_t m_NextRetry;

	bool Streaming() const;

	Spiral::Core::AudioTimeline m_Timeline;
};

#endif
