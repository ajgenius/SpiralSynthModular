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

// A device that moves samples across the shared transport. The hub tells
// it when the transport mode changes so it can report that to its GUI.
class AudioEndpoint : public AudioDriver
{
public:
	virtual void TransportModeChanged(int mode) {}

	// Tell the engine whether this stream now paces it.
	void Blocking(bool mode)
		{ if (cb_Blocking) cb_Blocking(m_Parent, mode); }

	void NotifyBufferAndSampleRate(unsigned long frames, unsigned long rate)
		{ if (ChangeBufferAndSampleRate) ChangeBufferAndSampleRate(frames, rate, m_Parent); }
};

// Owns the one running audio stream on behalf of every attached endpoint.
// The first endpoint attached is the representative: it carries the audio
// cycle callback and services the stream once per control cycle. The
// stream is configured on first attach and released on last detach.
class AudioTransportHub
{
public:
	enum Mode {NO_MODE,INPUT,OUTPUT,DUPLEX,CLOSED};

	static AudioTransportHub *Get();

	void Attach(AudioEndpoint *endpoint, const HostInfo *host);
	void Detach(AudioEndpoint *endpoint);
	bool IsRepresentative(const AudioEndpoint *endpoint) const
		{ return !m_Members.empty() && m_Members.front()==endpoint; }

	OutputAudioClient *Transport() const { return OUTPUTCLIENT::Get(); }
	bool IsCallbackDriven() const { return Transport()->IsCallbackDriven(); }
	Mode GetMode() const { return m_Mode; }

	void OpenMode(Mode mode);
	void Close();
	// Configure again from the host's current settings and reopen.
	void Reconfigure();

	// Control thread, representative only.
	void Service();
	// Audio cycle, representative only.
	void Process();

private:
	AudioTransportHub();
	static void ProcessCallback(void *context, unsigned int frames);
	void ReportMode();
	void Blocking(AudioEndpoint *endpoint, bool mode);

	static AudioTransportHub *m_Singleton;
	std::vector<AudioEndpoint *> m_Members;
	const HostInfo *m_Host;
	bool m_Configured;
	Mode m_Mode;
	Mode m_RequestedMode;
	bool m_IOFailed;
	time_t m_NextRetry;
};

#endif
