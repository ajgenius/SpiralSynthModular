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

#include "SpiralPlugin.h"
#include "OutputAudioClient.h"
#include <vector>
#include <ctime>
#include "PresentationClock.h"

class AtomicClock;

// A device that moves samples across the shared transport. The hub tells
// it when the transport mode changes so it can report that to its GUI.
class AudioEndpoint : public AudioDriver
{
public:
	virtual void TransportModeChanged(int mode) {}

	void NotifyBufferAndSampleRate(unsigned long frames, unsigned long rate)
		{ if (ChangeBufferAndSampleRate) ChangeBufferAndSampleRate(frames, rate, m_Parent); }
};

// One engine timeline shared by Output and named JACK clients. Control and
// engine access require the host cycle gate. Native callbacks touch only their
// AudioStream; no device pointer is retained across an engine sleep.
class AudioTransportHub
{
public:
	enum Mode {NO_MODE,INPUT,OUTPUT,DUPLEX,CLOSED};

	static AudioTransportHub *Get();

	// The engine supplies its format even when no Output endpoint exists.
	// Set or clear it before starting the engine, or while it is stopped.
	void SetHost(const HostInfo *host);

	void Attach(AudioEndpoint *endpoint, const HostInfo *host);
	void Detach(AudioEndpoint *endpoint);
	bool IsRepresentative(const AudioEndpoint *endpoint) const
		{ return !m_Members.empty() && m_Members.front()==endpoint; }

	OutputAudioClient *Transport() const { return OUTPUTCLIENT::Get(); }
	bool IsCallbackDriven() const { return Transport()->IsCallbackDriven(); }
	Mode GetMode() const { return m_Mode; }
	// Periods the transport played as silence because the engine was late.
	unsigned Underruns() const { return Transport()->Underruns(); }

	void OpenMode(Mode mode);
	void Close();
	// Configure again from the host's current settings and reopen.
	void Reconfigure();

	// Control thread, representative only.
	void Service();

	void RegisterStream(spiralcore::AudioStream *stream);
	void UnregisterStream(spiralcore::AudioStream *stream);
	bool PreparePeriod();
	bool PreparePeriod(double monotonicNow);
	unsigned SleepMicroseconds() const;
	// Convenience for single-threaded clients; the host uses PreparePeriod
	// under its gate and sleeps afterwards using a copied scalar duration.
	bool WaitPeriod();
	void BeginPeriod();
	void CommitPeriod();
	const spiralcore::AudioStamp &PlaybackStamp() const { return m_PlaybackStamp; }
	const spiralcore::AudioStamp &CaptureStamp() const { return m_CaptureStamp; }

	// Engine position: the frame the period being rendered starts at.
	// Slaved to the stream's transport when it has one (JACK), free
	// running otherwise; start, stop and locate ride the same clock.
	unsigned long Frame() const { return m_Frame; }
	bool Rolling() const { return m_Rolling; }
	void Start();
	void Stop();
	void Locate(unsigned long frame);

private:
	AudioTransportHub();
	void ReportMode();

	static AudioTransportHub *m_Singleton;
	std::vector<AudioEndpoint *> m_Members;
	const HostInfo *m_Host;
	bool m_Configured;
	Mode m_Mode;
	Mode m_RequestedMode;
	time_t m_NextRetry;

	unsigned long m_Frame;
	bool m_Rolling;
	bool Streaming() const;

	std::vector<spiralcore::AudioStream *> m_Streams;
	spiralcore::AudioStream *m_Master;
	spiralcore::PresentationClock m_Presentation;
	spiralcore::AudioStamp m_PlaybackStamp, m_CaptureStamp;
	spiralcore::AudioClient *MasterClient() const;
};

#endif
