// Copyright (C) 2003 David Griffiths <dave@pawfal.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifndef JACK_CLIENT
#define JACK_CLIENT

#include "AudioClient.h"
#include <map>
#include <vector>
#include <string>
#include <jack/jack.h>
#include <jack/transport.h>

namespace spiralcore
{
const int MAX_INPUTPORTS = 256;
const int MAX_OUTPUTPORTS = 256;

class JackClient : public AudioClient
{
public:
	JackClient();
	virtual ~JackClient();
	// Retain the original singleton API for existing libspiralcore callers.
	static JackClient *Get();
	static void PackUpAndGoHome();

	bool Attach(const std::string &clientName);
	virtual bool Attach(const std::string &clientName, const AudioClientOptions &options);
	virtual bool IsCallbackDriven() const { return true; }

	virtual bool Start();
	virtual bool Read(float *interleaved, unsigned int frames);
	virtual bool Write(const float *interleaved, unsigned int frames);
	void Detach();
	bool IsAttached() const { return __sync_fetch_and_add(&m_Attached, 0) != 0; }

	// Zero frames reports shutdown; engine/patch policy belongs to the caller.
	void SetCallback(void (*run)(void *, unsigned int), void *context);
	unsigned long GetBufferSize() const { return __sync_fetch_and_add(&m_BufferSize, 0); }

	unsigned long GetSampleRate() const { return __sync_fetch_and_add(&m_SampleRate, 0); }

	bool GetCycleTiming(AudioCycleTiming &timing) const;
	double GetChannelTime(bool input, unsigned channel) const;
	double GetInputLatency() const;
	double GetOutputLatency() const;

	// Transport is optional application policy; attaching never starts it.
	bool QueryTransport(jack_transport_state_t &state, jack_position_t &position) const;
	virtual bool GetTransport(unsigned long &frame, bool &rolling) const;

	virtual bool StartTransport();

	virtual bool StopTransport();

	virtual bool LocateTransport(unsigned long frame);

	int AddInputPort();

	int AddOutputPort();

	int AddInputPort(int id);

	int AddOutputPort(int id);
	void RemoveInputPort(int id);
	void RemoveOutputPort(int id);
	void GetPortNames(std::vector<std::string> &inputs, std::vector<std::string> &outputs);
	void ConnectInput(int id, const std::string &port);
	void ConnectOutput(int id, const std::string &port);
	void DisconnectInput(int id);
	void DisconnectOutput(int id);
	std::string GetInputName(int id) const;

	std::string GetOutputName(int id) const;

	std::string GetInputConnection(int id) const;

	std::string GetOutputConnection(int id) const;
	void SetInputBuf(int id, float *buffer, unsigned int frames = 0);
	void SetOutputBuf(int id, float *buffer, unsigned int frames = 0);

private:
	struct JackPort
	{
		JackPort() : Buffer(NULL), Frames(0), Port(NULL) {}

		std::string Name;
		float *Buffer;
		unsigned int Frames;
		jack_port_t *Port;
	};

	typedef std::map<int, JackPort> PortMap;
	int AddPort(PortMap &ports, int &nextID, bool input);
	void RemovePort(PortMap &ports, int id);
	void DetachClient();
	bool StartClient();

	bool RegisterPorts(PortMap &ports, bool input);
	void SetBuffer(PortMap &ports, int id, float *buffer, unsigned int frames);
	std::string Connection(const PortMap &ports, int id) const;
	void Connect(PortMap &ports, int id, const std::string &port, bool input);
	void Disconnect(PortMap &ports, int id);
	static int Process(jack_nframes_t frames, void *context);
	static int OnSRateChange(jack_nframes_t rate, void *context);
	static int OnBufferSizeChange(jack_nframes_t frames, void *context);
	static void OnJackShutdown(void *context);

	static JackClient *m_Singleton;
	jack_client_t *m_Client;
	PortMap m_InputPortMap;
	PortMap m_OutputPortMap;
	mutable unsigned long m_BufferSize;

	mutable unsigned long m_SampleRate;
	mutable int m_Attached;
	bool m_AutoActivate;

	bool m_Active;

	unsigned int m_ProcessFrames;

	AudioCycleTiming m_Timing;
	uint64_t m_NativeFrame;
	jack_nframes_t m_LastFrame;
	bool m_HaveFrame;
	double m_CycleTime;
	double PortLatency(const JackPort &port, bool input) const;
	double MaximumLatency(const PortMap &ports, bool input) const;

	int m_NextInputID;

	int m_NextOutputID;
	void (*m_Run)(void *, unsigned int);
	void *m_Context;
	JackClient(const JackClient &);
	JackClient &operator=(const JackClient &);
};

}

#endif
