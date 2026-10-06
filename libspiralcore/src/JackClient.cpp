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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "JackClient.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <pthread.h>

using namespace spiralcore;
using namespace std;

JackClient *JackClient::m_Singleton = NULL;

namespace
{
	pthread_mutex_t ClientLifecycleMutex = PTHREAD_MUTEX_INITIALIZER;
	vector<JackClient *> Clients;

	class ClientLifecycleLock
	{
	public:
		ClientLifecycleLock() { pthread_mutex_lock(&ClientLifecycleMutex); }

		~ClientLifecycleLock() { pthread_mutex_unlock(&ClientLifecycleMutex); }

	};

}

static void ReleasePortList(const char **names)
{
#ifdef HAVE_JACK_FREE
	jack_free(names);
#else
	free(names);
#endif
}

JackClient *JackClient::Get()
{
	if (!m_Singleton) m_Singleton = new JackClient;

	return m_Singleton;
}

void JackClient::PackUpAndGoHome()
{
	delete m_Singleton;
	m_Singleton = NULL;
}

JackClient::JackClient() :
	m_Client(NULL), m_BufferSize(0), m_SampleRate(0), m_Attached(false),
	m_AutoActivate(true), m_Active(false), m_ProcessFrames(0),
	m_NextInputID(0), m_NextOutputID(0), m_Run(NULL), m_Context(NULL)
{
	ClientLifecycleLock lock;
	Clients.push_back(this);
}

JackClient::~JackClient()
{
	ClientLifecycleLock lock;
	DetachClient();
	Clients.erase(remove(Clients.begin(), Clients.end(), this), Clients.end());
}

void JackClient::SetCallback(void (*run)(void *, unsigned int), void *context)
{
	m_Run = run;
	m_Context = context;
}

bool JackClient::RegisterPorts(PortMap &ports, bool input)
{
	for (PortMap::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		i->second.Port = jack_port_register(m_Client, i->second.Name.c_str(),
			JACK_DEFAULT_AUDIO_TYPE, input ? JackPortIsInput : JackPortIsOutput, 0);
		if (!i->second.Port) return false;

	}

	return true;
}

bool JackClient::Attach(const string &clientName)
{
	ClientLifecycleLock lock;
	if (IsAttached()) return true;

	// A server shutdown leaves a handle that still needs to be closed.
	DetachClient();
	// A failed open after server shutdown can make libjack discard its
	// remaining clients. Retire every stale handle before attempting it,
	// so another wrapper cannot later close an already destroyed client.
	for (size_t n=0; n<Clients.size(); ++n)
		if (Clients[n]->m_Client && !Clients[n]->IsAttached())
			Clients[n]->DetachClient();

#ifdef HAVE_JACK_CLIENT_OPEN
	m_Client = jack_client_open(clientName.c_str(),
		static_cast<jack_options_t>(JackNoStartServer | JackUseExactName), NULL);
#else
	m_Client = jack_client_new(clientName.c_str());
#endif
	if (!m_Client) return false;

	m_BufferSize = jack_get_buffer_size(m_Client);
	m_SampleRate = jack_get_sample_rate(m_Client);
	if (jack_set_process_callback(m_Client, Process, this) ||
		jack_set_sample_rate_callback(m_Client, OnSRateChange, this) ||
		jack_set_buffer_size_callback(m_Client, OnBufferSizeChange, this))
	{
		DetachClient();
		return false;
	}

	jack_on_shutdown(m_Client, OnJackShutdown, this);
	if (!RegisterPorts(m_InputPortMap, true) || !RegisterPorts(m_OutputPortMap, false))
	{
		DetachClient();
		return false;
	}

	__sync_lock_test_and_set(&m_Attached, 1);
	return !m_AutoActivate || StartClient();
}

void JackClient::Detach()
{
	ClientLifecycleLock lock;
	DetachClient();
}

void JackClient::DetachClient()
{
	bool notify = IsAttached();
	if (m_Client)
	{
		// Closing waits for process callbacks before releasing native ports.
		jack_client_close(m_Client);
		m_Client = NULL;
	}

	__sync_lock_test_and_set(&m_Attached, 0);
	m_Active = false;
	m_ProcessFrames = 0;
	for (PortMap::iterator i = m_InputPortMap.begin(); i != m_InputPortMap.end(); ++i)
	{
		i->second.Port = NULL;
		i->second.Buffer = NULL;
	}

	for (PortMap::iterator i = m_OutputPortMap.begin(); i != m_OutputPortMap.end(); ++i)
	{
		i->second.Port = NULL;
		i->second.Buffer = NULL;
	}

	if (notify && m_Run) m_Run(m_Context, 0);

}

int JackClient::Process(jack_nframes_t frames, void *context)
{
	JackClient *client = static_cast<JackClient *>(context);
	for (PortMap::iterator i = client->m_InputPortMap.begin(); i != client->m_InputPortMap.end(); ++i)
	{
		JackPort &port = i->second;
		if (!port.Buffer || !port.Port) continue;

		if (frames <= port.Frames && jack_port_connected(port.Port))
			memcpy(port.Buffer, jack_port_get_buffer(port.Port, frames), sizeof(float) * frames);

		else
			memset(port.Buffer, 0, sizeof(float) * port.Frames);
	}

	for (PortMap::iterator i = client->m_OutputPortMap.begin(); i != client->m_OutputPortMap.end(); ++i)
	{
		JackPort &port = i->second;
		if (!port.Port) continue;

		float *output = static_cast<float *>(jack_port_get_buffer(port.Port, frames));
		if (port.Buffer && frames <= port.Frames)
			memcpy(output, port.Buffer, sizeof(float) * frames);

		else
			memset(output, 0, sizeof(float) * frames);
	}

	client->m_BufferSize = frames;
	client->m_ProcessFrames = frames;
	if (client->m_Run) client->m_Run(client->m_Context, frames);

	client->m_ProcessFrames = 0;
	return 0;
}

int JackClient::OnSRateChange(jack_nframes_t rate, void *context)
{
	static_cast<JackClient *>(context)->m_SampleRate = rate;
	return 0;
}

int JackClient::OnBufferSizeChange(jack_nframes_t frames, void *context)
{
	static_cast<JackClient *>(context)->m_BufferSize = frames;
	return 0;
}

void JackClient::OnJackShutdown(void *context)
{
	JackClient *client = static_cast<JackClient *>(context);
	__sync_lock_test_and_set(&client->m_Attached, 0);
	client->m_Active = false;
	if (client->m_Run) client->m_Run(client->m_Context, 0);

}

int JackClient::AddPort(PortMap &ports, int &nextID, bool input)
{
	char name[32];
	sprintf(name, "%s%d", input ? "In" : "Out", nextID);
	JackPort port;
	port.Name = name;
	bool active = m_Active;
	if (active && jack_deactivate(m_Client)) return -1;

	if (m_Client)
	{
		port.Port = jack_port_register(m_Client, name, JACK_DEFAULT_AUDIO_TYPE,
			input ? JackPortIsInput : JackPortIsOutput, 0);
		if (!port.Port)
		{
			if (active && jack_activate(m_Client)) Detach();

			return -1;
		}

	}

	ports[nextID] = port;
	int id = nextID++;
	if (active && jack_activate(m_Client))
	{
		Detach();
		return -1;
	}

	return id;
}

int JackClient::AddInputPort() { return AddPort(m_InputPortMap, m_NextInputID, true); }

int JackClient::AddOutputPort() { return AddPort(m_OutputPortMap, m_NextOutputID, false); }

int JackClient::AddInputPort(int id)
{
	if (m_InputPortMap.find(id) != m_InputPortMap.end()) return id;

	int next = id;

	int result = AddPort(m_InputPortMap, next, true);
	if (result >= m_NextInputID) m_NextInputID = result + 1;

	return result;
}

int JackClient::AddOutputPort(int id)
{
	if (m_OutputPortMap.find(id) != m_OutputPortMap.end()) return id;

	int next = id;

	int result = AddPort(m_OutputPortMap, next, false);
	if (result >= m_NextOutputID) m_NextOutputID = result + 1;

	return result;
}

void JackClient::RemovePort(PortMap &ports, int id)
{
	PortMap::iterator i = ports.find(id);
	if (i == ports.end()) return;

	// Port maps are traversed by Process; stop callbacks before changing them.
	bool active = m_Active;
	if (active && jack_deactivate(m_Client)) return;

	if (m_Client && i->second.Port) jack_port_unregister(m_Client, i->second.Port);

	ports.erase(i);
	if (active && jack_activate(m_Client)) Detach();

}

void JackClient::RemoveInputPort(int id) { RemovePort(m_InputPortMap, id); }

void JackClient::RemoveOutputPort(int id) { RemovePort(m_OutputPortMap, id); }

void JackClient::GetPortNames(vector<string> &inputs, vector<string> &outputs)
{
	inputs.clear();
	outputs.clear();
	if (!IsAttached()) return;

	const char **names = jack_get_ports(m_Client, NULL, NULL, JackPortIsInput);
	if (names)
	{
		for (int i = 0; names[i]; ++i) inputs.push_back(names[i]);

		ReleasePortList(names);
	}

	names = jack_get_ports(m_Client, NULL, NULL, JackPortIsOutput);
	if (names)
	{
		for (int i = 0; names[i]; ++i) outputs.push_back(names[i]);

		ReleasePortList(names);
	}

}

string JackClient::GetInputName(int id) const
{
	PortMap::const_iterator i = m_InputPortMap.find(id);
	return i == m_InputPortMap.end() ? string() : i->second.Name;
}

string JackClient::GetOutputName(int id) const
{
	PortMap::const_iterator i = m_OutputPortMap.find(id);
	return i == m_OutputPortMap.end() ? string() : i->second.Name;
}

string JackClient::Connection(const PortMap &ports, int id) const
{
	PortMap::const_iterator i = ports.find(id);
	if (!IsAttached() || i == ports.end() || !i->second.Port) return string();

	const char **names = jack_port_get_all_connections(m_Client, i->second.Port);
	string name = names && names[0] ? names[0] : "";
	ReleasePortList(names);
	return name;
}

string JackClient::GetInputConnection(int id) const { return Connection(m_InputPortMap, id); }

string JackClient::GetOutputConnection(int id) const { return Connection(m_OutputPortMap, id); }

void JackClient::Disconnect(PortMap &ports, int id)
{
	PortMap::iterator i = ports.find(id);
	if (IsAttached() && i != ports.end() && i->second.Port)
		jack_port_disconnect(m_Client, i->second.Port);

}

void JackClient::Connect(PortMap &ports, int id, const string &name, bool input)
{
	PortMap::iterator i = ports.find(id);
	if (!IsAttached() || i == ports.end() || !i->second.Port) return;

	Disconnect(ports, id);
	const char *local = jack_port_name(i->second.Port);
	int error = input ? jack_connect(m_Client, name.c_str(), local)
		: jack_connect(m_Client, local, name.c_str());
	if (error) cerr << "JACK: cannot connect " << local << " to " << name << endl;

}

void JackClient::ConnectInput(int id, const string &port) { Connect(m_InputPortMap, id, port, true); }

void JackClient::ConnectOutput(int id, const string &port) { Connect(m_OutputPortMap, id, port, false); }

void JackClient::DisconnectInput(int id) { Disconnect(m_InputPortMap, id); }

void JackClient::DisconnectOutput(int id) { Disconnect(m_OutputPortMap, id); }

void JackClient::SetBuffer(PortMap &ports, int id, float *buffer, unsigned int frames)
{
	PortMap::iterator i = ports.find(id);
	if (i == ports.end()) return;

	i->second.Buffer = buffer;
	i->second.Frames = frames ? frames : m_BufferSize;
}

void JackClient::SetInputBuf(int id, float *buffer, unsigned int frames)
{
	SetBuffer(m_InputPortMap, id, buffer, frames);
}

void JackClient::SetOutputBuf(int id, float *buffer, unsigned int frames)
{
	SetBuffer(m_OutputPortMap, id, buffer, frames);
}

// These requests do not change engine pause state. A host may opt into transport
// synchronization separately, without making every audio client a transport owner.
bool JackClient::QueryTransport(jack_transport_state_t &state, jack_position_t &position) const
{
	if (!IsAttached() || !m_Client)
		return false;

	state = jack_transport_query(m_Client, &position);
	return true;
}

bool JackClient::GetTransport(unsigned long &frame, bool &rolling) const
{
	jack_transport_state_t state;
	jack_position_t position;
	if (!QueryTransport(state, position)) return false;

	frame = position.frame;
	rolling = state == JackTransportRolling;
	return true;
}

bool JackClient::StartTransport()
{
	if (!IsAttached() || !m_Client)
		return false;

	jack_transport_start(m_Client);
	return true;
}

bool JackClient::StopTransport()
{
	if (!IsAttached() || !m_Client)
		return false;

	jack_transport_stop(m_Client);
	return true;
}

bool JackClient::LocateTransport(unsigned long frame)
{
	return IsAttached() && m_Client && jack_transport_locate(m_Client, (jack_nframes_t)frame) == 0;
}


bool JackClient::Attach(const string &clientName, const AudioClientOptions &options)
{
	Detach();
	if (options.InChannels > MAX_INPUTPORTS || options.OutChannels > MAX_OUTPUTPORTS ||
		(!options.InChannels && !options.OutChannels))
		return false;

	m_InputPortMap.clear();
	m_OutputPortMap.clear();
	m_NextInputID = m_NextOutputID = 0;
	for (unsigned int i = 0; i < options.InChannels; ++i)
		AddInputPort();

	for (unsigned int i = 0; i < options.OutChannels; ++i)
		AddOutputPort();

	m_AutoActivate = false;
	const bool opened = Attach(clientName);
	m_AutoActivate = true;
	return opened;
}

bool JackClient::Start()
{
	ClientLifecycleLock lock;
	return StartClient();
}

bool JackClient::StartClient()
{
	if (!IsAttached() || !m_Client)
		return false;

	if (m_Active)
		return true;

	if (jack_activate(m_Client))
	{
		DetachClient();
		return false;
	}

	m_Active = true;
	return true;
}

bool JackClient::Read(float *interleaved, unsigned int frames)
{
	if (!interleaved || !frames || frames != m_ProcessFrames)
		return false;

	const size_t channels = m_InputPortMap.size();

	size_t channel = 0;
	for (PortMap::iterator i = m_InputPortMap.begin(); i != m_InputPortMap.end(); ++i, ++channel)
	{
		const float *input = static_cast<const float *>(jack_port_get_buffer(i->second.Port, frames));
		for (unsigned int frame = 0; frame < frames; ++frame)
			interleaved[frame * channels + channel] = input[frame];

	}

	return true;
}

bool JackClient::Write(const float *interleaved, unsigned int frames)
{
	if (!interleaved || !frames || frames != m_ProcessFrames)
		return false;

	const size_t channels = m_OutputPortMap.size();

	size_t channel = 0;
	for (PortMap::iterator i = m_OutputPortMap.begin(); i != m_OutputPortMap.end(); ++i, ++channel)
	{
		float *output = static_cast<float *>(jack_port_get_buffer(i->second.Port, frames));
		for (unsigned int frame = 0; frame < frames; ++frame)
			output[frame] = interleaved[frame * channels + channel];

	}

	return true;
}
