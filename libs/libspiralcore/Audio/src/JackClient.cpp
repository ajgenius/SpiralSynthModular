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
#include <sched.h>
#include <deque>

using namespace Spiral::Core;
using namespace std;

JackClient *JackClient::m_Singleton = NULL;

namespace
{
	pthread_mutex_t ClientLifecycleMutex = PTHREAD_MUTEX_INITIALIZER;
	vector<JackClient *> Clients;
	double ClockOffset = 0;
	bool ClockMapped = false;

	void MapClock()
	{
		if (ClockMapped) return;

		double best = 1;
		for (unsigned n = 0; n < 8; ++n)
		{
			const double before = AudioMonotonicTime();
			const double native = jack_get_time() * 1e-6;
			const double after = AudioMonotonicTime();
			if (after - before < best)
			{
				best = after - before;
				ClockOffset = (before + after) * 0.5 - native;
			}

		}

		ClockMapped = true;
	}

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

// Each group owns its stream and ports. JACK calls one physical client. A control
// edit withdraws only the affected group and waits out its last callback.
namespace Spiral
{
namespace Core
{
struct JackSession
{
	enum { MaxGroups = 256 };
	const void *owner;
	JackClient clock;
	JackClient *members[MaxGroups];
	JackClient *running[MaxGroups];
	unsigned readers, limit;
	string name;
	bool pumping;
	int prepared;
	double nextTry;
	bool NativeReady() const
	{
		return __atomic_load_n(&prepared, __ATOMIC_ACQUIRE) && clock.IsAttached();
	}

	JackSession(const void *engine, const string &destination):
	    owner(engine), readers(0), limit(0), name(destination), pumping(false), prepared(0), nextTry(0)
	{
		memset(members, 0, sizeof(members));
		memset(running, 0, sizeof(running));
	}

	static void Run(void *context, unsigned frames)
	{
		JackSession &s = *static_cast<JackSession *>(context);
		if (!frames) return;

		__atomic_add_fetch(&s.readers, 1, __ATOMIC_SEQ_CST);
		const unsigned limit = __atomic_load_n(&s.limit, __ATOMIC_ACQUIRE);
		for (unsigned i = 0; i < limit; ++i)
		{
			JackClient *group = __atomic_load_n(&s.running[i], __ATOMIC_SEQ_CST);
			if (!group) continue;

			group->ClearBuffers(frames);
			group->m_CycleTime = s.clock.m_CycleTime;
			group->m_Timing = s.clock.m_Timing;
			group->m_Timing.InputTime -= group->GetInputLatency();
			group->m_Timing.OutputTime += group->GetOutputLatency();
			group->m_ProcessFrames = frames;
			if (group->m_Run) group->m_Run(group->m_Context, frames);
			group->m_ProcessFrames = 0;
		}
		__atomic_sub_fetch(&s.readers, 1, __ATOMIC_SEQ_CST);
	}
};
}
}

namespace
{
	pthread_mutex_t SessionMutex = PTHREAD_MUTEX_INITIALIZER;
	vector<JackSession *> Sessions;
	struct RouteRequest
	{
		JackClient *client;
		int port;
		unsigned ticket;
		string peer;
		bool connect;
	};
	std::deque<RouteRequest> routes;
	pthread_mutex_t routeMutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t routeIdle = PTHREAD_COND_INITIALIZER;
	JackClient *routing = NULL;

	struct SessionLock
	{
		SessionLock() { pthread_mutex_lock(&SessionMutex); }
		~SessionLock() { pthread_mutex_unlock(&SessionMutex); }
	};
}

// SessionMutex protects the reservation list. A slow native open owns a pin and
// runs with neither this mutex nor the synth graph gate held.
static void ReleaseEmptySession(JackSession *s)
{
	if (s->pumping) return;
	for (unsigned n = 0; n < s->limit; ++n)
		if (s->members[n]) return;
	Sessions.erase(remove(Sessions.begin(), Sessions.end(), s), Sessions.end());
	delete s;
}

void JackClient::RequestRoutes(int port, unsigned ticket, const string &peer, bool connect)
{
	if (!IsAttached() || port == 0 || port < -64 || port > 64) return;
	const RouteRequest request = {this, port, ticket, peer, connect};
	pthread_mutex_lock(&routeMutex);
	routes.push_back(request);
	pthread_mutex_unlock(&routeMutex);
}

bool JackClient::TakeRoutes(string &result)
{
	if (!__atomic_load_n(&m_HaveRouteReply, __ATOMIC_ACQUIRE)) return false;
	pthread_mutex_lock(&routeMutex);
	const bool ready = !m_RouteReply.empty();
	if (ready)
	{
		result.swap(m_RouteReply);
		__atomic_store_n(&m_HaveRouteReply, 0, __ATOMIC_RELEASE);
		m_RouteReply.clear();
	}
	pthread_mutex_unlock(&routeMutex);
	return ready;
}

void JackClient::WithdrawRoutes()
{
	pthread_mutex_lock(&routeMutex);
	for (std::deque<RouteRequest>::iterator i = routes.begin(); i != routes.end();)
	{
		if (i->client == this) i = routes.erase(i);
		else ++i;
	}
	while (routing == this) pthread_cond_wait(&routeIdle, &routeMutex);
	__atomic_store_n(&m_HaveRouteReply, 0, __ATOMIC_RELEASE);
	m_RouteReply.clear();
	pthread_mutex_unlock(&routeMutex);
}

void JackClient::ServiceRoutes()
{
	for (;;)
	{
		pthread_mutex_lock(&routeMutex);
		if (routing || routes.empty())
		{
			pthread_mutex_unlock(&routeMutex);
			return;
		}
		const RouteRequest request = routes.front();
		routes.pop_front();
		routing = request.client;
		pthread_mutex_unlock(&routeMutex);

		const bool input = request.port < 0;
		const int channel = (input ? -request.port : request.port) - 1;
		JackClient *client = request.client;
		if (!request.peer.empty())
			client->SetPeerConnected(input, channel, request.peer, request.connect);

		vector<string> inputs, outputs;
		client->GetPortNames(inputs, outputs);
		const vector<string> &names = input ? outputs : inputs;
		const vector<string> peers = client->Peers(input, channel);
		char number[32];
		snprintf(number, sizeof(number), "%d %u", request.port, request.ticket);
		string reply(number);
		reply += char(0);
		for (size_t n = 0; n < names.size(); ++n)
		{
			reply += find(peers.begin(), peers.end(), names[n]) == peers.end() ? '+' : '-';
			reply += names[n];
			reply += char(0);
		}

		pthread_mutex_lock(&routeMutex);
		client->m_RouteReply.swap(reply);
		__atomic_store_n(&client->m_HaveRouteReply, 1, __ATOMIC_RELEASE);
		routing = NULL;
		pthread_cond_broadcast(&routeIdle);
		pthread_mutex_unlock(&routeMutex);
	}
}

void JackClient::ServiceSessions()
{
	ServiceRoutes();
	for (;;)
	{
		JackSession *s = NULL;
		{
			SessionLock lock;
			const double now = AudioMonotonicTime();
			for (size_t n = 0; n < Sessions.size(); ++n)
			{
				JackSession *candidate = Sessions[n];
				if (!candidate->prepared && !candidate->pumping && candidate->nextTry <= now)
				{
					candidate->pumping = true;
					s = candidate;
					break;
				}
			}
		}
		if (!s) return;

		const bool opened = s->clock.Attach(s->name);
		SessionLock lock;
		s->nextTry = AudioMonotonicTime() + 1;
		s->pumping = false;
		__atomic_store_n(&s->prepared, opened ? 1 : 0, __ATOMIC_RELEASE);
		ReleaseEmptySession(s);
	}
}

bool JackClient::WaitingForSession() const
{
	return m_Session && m_DeferOpen && !m_PortsReady;
}

bool JackClient::SessionReady() const
{
	return WaitingForSession() && __atomic_load_n(&m_Session->prepared, __ATOMIC_ACQUIRE);
}

void JackClient::SetPortGroup(const void *owner, const string &prefix, bool deferOpen, bool exactName)
{
	m_Owner = owner;
	m_Prefix = prefix;
	m_DeferOpen = deferOpen;
	m_ExactName = exactName;
}

jack_client_t *JackClient::Handle() const
{
	return m_Session ? m_Session->clock.m_Client : m_Client;
}

bool JackClient::IsAttached() const
{
	return m_Session ? m_PortsReady && m_Session->NativeReady() : __sync_fetch_and_add(&m_Attached, 0) != 0;
}

unsigned long JackClient::GetBufferSize() const
{
	return m_Session ? m_Session->clock.GetBufferSize() : __sync_fetch_and_add(&m_BufferSize, 0);
}

unsigned long JackClient::GetSampleRate() const
{
	return m_Session ? m_Session->clock.GetSampleRate() : __sync_fetch_and_add(&m_SampleRate, 0);
}

void JackClient::SuspendPortGroup()
{
	if (!m_Session) return;
	WithdrawRoutes();
	__atomic_store_n(&m_Session->running[m_Slot], static_cast<JackClient *>(NULL), __ATOMIC_SEQ_CST);
	while (__atomic_load_n(&m_Session->readers, __ATOMIC_SEQ_CST))
		sched_yield();
}

bool JackClient::ResizePortGroup(unsigned inputs, unsigned outputs)
{
	if (!m_Session || !m_Session->NativeReady() || inputs > MAX_INPUTPORTS || outputs > MAX_OUTPUTPORTS)
		return false;

	SuspendPortGroup();
	PortMap *maps[] = {&m_InputPortMap, &m_OutputPortMap};
	const unsigned old[] = {unsigned(m_InputPortMap.size()), unsigned(m_OutputPortMap.size())};
	const unsigned sizes[] = {inputs, outputs};
	bool ok = true;
	for (unsigned dir = 0; ok && dir < 2; ++dir)
	{
		for (unsigned n = old[dir]; n < sizes[dir]; ++n)
		{
			char suffix[32];
			snprintf(suffix, sizeof(suffix), "%s%u", dir ? "Out" : "In", n);
			JackPort port;
			port.Name = m_Prefix + suffix;
			port.Port = jack_port_register(Handle(), port.Name.c_str(), JACK_DEFAULT_AUDIO_TYPE,
				dir ? JackPortIsOutput : JackPortIsInput, 0);
			if (!port.Port)
			{
				ok = false;
				break;
			}
			(*maps[dir])[int(n)] = port;
		}
	}

	// Remove trailing ports only. Failed growth rolls back both directions.
	for (unsigned dir = 0; dir < 2; ++dir)
	{
		PortMap &ports = *maps[dir];
		while (ports.size() > (ok ? sizes[dir] : old[dir]))
		{
			PortMap::iterator last = --ports.end();
			jack_port_unregister(Handle(), last->second.Port);
			ports.erase(last);
		}
	}
	return ok;
}

bool JackClient::AttachGroup(const string &name, unsigned inputs, unsigned outputs)
{
	{
		SessionLock lock;
		for (size_t n = 0; n < Sessions.size(); ++n)
			if (Sessions[n]->owner == m_Owner && (!Sessions[n]->prepared || Sessions[n]->NativeReady()))
			{
				m_Session = Sessions[n];
				break;
			}

		if (!m_Session)
		{
			JackSession *s = new JackSession(m_Owner, name);
			s->clock.m_AutoActivate = false;
			s->clock.m_ExactName = m_ExactName;
			if (!m_DeferOpen && !s->clock.Attach(name))
			{
				delete s;
				return false;
			}
			s->prepared = m_DeferOpen ? 0 : 1;
			s->clock.SetCallback(JackSession::Run, s);
			Sessions.push_back(s);
			m_Session = s;
		}

		m_Slot = 0;
		while (m_Slot < JackSession::MaxGroups && m_Session->members[m_Slot]) ++m_Slot;
		if (m_Slot == JackSession::MaxGroups)
		{
			m_Session = NULL;
			return false;
		}
		m_Session->members[m_Slot] = this;
		if (m_Slot >= m_Session->limit)
			__atomic_store_n(&m_Session->limit, m_Slot + 1, __ATOMIC_RELEASE);
	}

	m_InputPortMap.clear();
	m_OutputPortMap.clear();
	if (!m_Session->NativeReady()) return false;
	if (ResizePortGroup(inputs, outputs)) return m_PortsReady = true;
	DetachGroup();
	return false;
}

void JackClient::DetachGroup()
{
	SessionLock lock;
	SuspendPortGroup();
	if (IsAttached())
		ResizePortGroup(0, 0);
	m_InputPortMap.clear();
	m_OutputPortMap.clear();
	JackSession *s = m_Session;
	m_Session = NULL;
	m_PortsReady = false;
	s->members[m_Slot] = NULL;
	ReleaseEmptySession(s);
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
	m_Owner(NULL), m_Session(NULL), m_Slot(0), m_DeferOpen(false), m_PortsReady(false), m_ExactName(true), m_HaveRouteReply(0), m_Client(NULL), m_BufferSize(0), m_SampleRate(0), m_Attached(false),
	m_AutoActivate(true), m_Active(false), m_ProcessFrames(0),
	m_NativeFrame(0), m_LastFrame(0), m_HaveFrame(false), m_CycleTime(0),
	m_NextInputID(0), m_NextOutputID(0), m_Run(NULL), m_Context(NULL)
{
	ClientLifecycleLock lock;
	Clients.push_back(this);
}

JackClient::~JackClient()
{
	WithdrawRoutes();
	if (m_Session) DetachGroup();
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

	DetachClient();
	for (size_t n=0; n<Clients.size(); ++n)
		if (Clients[n]->m_Client && !Clients[n]->IsAttached())
			Clients[n]->DetachClient();

#ifdef HAVE_JACK_CLIENT_OPEN
	m_Client = jack_client_open(clientName.c_str(),
		static_cast<jack_options_t>(JackNoStartServer | (m_ExactName ? JackUseExactName : 0)), NULL);
#else
	m_Client = jack_client_new(clientName.c_str());
#endif
	if (!m_Client) return false;

	MapClock();
	m_HaveFrame = false;
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
	WithdrawRoutes();
	if (m_Session)
	{
		DetachGroup();
		return;
	}

	ClientLifecycleLock lock;
	DetachClient();
}

void JackClient::DetachClient()
{
	bool notify = IsAttached();
	if (m_Client)
	{
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

void JackClient::ClearBuffers(unsigned frames)
{
	for (PortMap::iterator i = m_InputPortMap.begin(); i != m_InputPortMap.end(); ++i)
	{
		JackPort &port = i->second;
		if (!port.Buffer || !port.Port) continue;

		if (frames <= port.Frames && jack_port_connected(port.Port))
			memcpy(port.Buffer, jack_port_get_buffer(port.Port, frames), sizeof(float) * frames);

		else
			memset(port.Buffer, 0, sizeof(float) * port.Frames);
	}

	for (PortMap::iterator i = m_OutputPortMap.begin(); i != m_OutputPortMap.end(); ++i)
	{
		JackPort &port = i->second;
		if (!port.Port) continue;

		float *output = static_cast<float *>(jack_port_get_buffer(port.Port, frames));
		if (port.Buffer && frames <= port.Frames)
			memcpy(output, port.Buffer, sizeof(float) * frames);

		else
			memset(output, 0, sizeof(float) * frames);
	}
}

int JackClient::Process(jack_nframes_t frames, void *context)
{
	JackClient *client = static_cast<JackClient *>(context);
	client->ClearBuffers(frames);

	__sync_lock_test_and_set(&client->m_BufferSize, frames);
	jack_nframes_t frame;
	jack_time_t current, next;
#ifdef HAVE_JACK_GET_CYCLE_TIMES
	float periodUsecs;

	// Keep the frame and both timestamps from one server cycle.
	if (jack_get_cycle_times(client->m_Client, &frame, &current, &next, &periodUsecs) != 0)
#endif
	{
		frame = jack_last_frame_time(client->m_Client);
		current = jack_frames_to_time(client->m_Client, frame);
		next = jack_frames_to_time(client->m_Client, frame + frames);
	}

	client->m_NativeFrame = client->m_HaveFrame
		? client->m_NativeFrame + jack_nframes_t(frame - client->m_LastFrame) : frame;
	client->m_LastFrame = frame;
	client->m_HaveFrame = true;
	client->m_CycleTime = current * 1e-6 + ClockOffset;
	client->m_Timing.Step = next > current && frames ? double(next - current) * 1e-6 / frames : 0;
	client->m_Timing.Frame = client->m_NativeFrame;
	client->m_Timing.Frames = frames;
	client->m_Timing.SampleRate = client->GetSampleRate();
	client->m_Timing.CallbackTime = AudioMonotonicTime();
	client->m_Timing.InputTime = client->m_CycleTime - client->GetInputLatency();
	client->m_Timing.OutputTime = client->m_CycleTime + client->GetOutputLatency();
	client->m_Timing.Valid = client->m_Timing.SampleRate > 0;
	client->m_ProcessFrames = frames;
	if (client->m_Run) client->m_Run(client->m_Context, frames);

	client->m_ProcessFrames = 0;
	return 0;
}

int JackClient::OnSRateChange(jack_nframes_t rate, void *context)
{
	__sync_lock_test_and_set(&static_cast<JackClient *>(context)->m_SampleRate, rate);
	return 0;
}

int JackClient::OnBufferSizeChange(jack_nframes_t frames, void *context)
{
	__sync_lock_test_and_set(&static_cast<JackClient *>(context)->m_BufferSize, frames);
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

	const char **names = jack_get_ports(Handle(), NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
	if (names)
	{
		for (int i = 0; names[i]; ++i) inputs.push_back(names[i]);

		ReleasePortList(names);
	}

	names = jack_get_ports(Handle(), NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
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

vector<string> JackClient::Peers(bool input, int id) const
{
	vector<string> peers;
	const PortMap &ports = input ? m_InputPortMap : m_OutputPortMap;
	PortMap::const_iterator port = ports.find(id);
	if (!IsAttached() || port == ports.end() || !port->second.Port) return peers;

	const char **names = jack_port_get_all_connections(Handle(), port->second.Port);
	if (names)
	{
		for (size_t i = 0; names[i]; ++i) peers.push_back(names[i]);
		ReleasePortList(names);
	}
	return peers;
}

bool JackClient::SetPeerConnected(bool input, int id, const string &peer, bool connected)
{
	const PortMap &ports = input ? m_InputPortMap : m_OutputPortMap;
	PortMap::const_iterator port = ports.find(id);
	if (!IsAttached() || peer.empty() || port == ports.end() || !port->second.Port) return false;

	const char *local = jack_port_name(port->second.Port);
	const char *from = input ? peer.c_str() : local;
	const char *to = input ? local : peer.c_str();
	const int result = connected ? jack_connect(Handle(), from, to) : jack_disconnect(Handle(), from, to);
	if (result == 0) return true;

	const vector<string> peers = Peers(input, id);
	return (find(peers.begin(), peers.end(), peer) != peers.end()) == connected;
}

string JackClient::Connection(const PortMap &ports, int id) const
{
	PortMap::const_iterator i = ports.find(id);
	if (!IsAttached() || i == ports.end() || !i->second.Port) return string();

	const char **names = jack_port_get_all_connections(Handle(), i->second.Port);
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
		jack_port_disconnect(Handle(), i->second.Port);

}

void JackClient::Connect(PortMap &ports, int id, const string &name, bool input)
{
	PortMap::iterator i = ports.find(id);
	if (!IsAttached() || i == ports.end() || !i->second.Port) return;

	Disconnect(ports, id);
	const char *local = jack_port_name(i->second.Port);
	int error = input ? jack_connect(Handle(), name.c_str(), local)
		: jack_connect(Handle(), local, name.c_str());
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

bool JackClient::QueryTransport(jack_transport_state_t &state, jack_position_t &position) const
{
	if (!IsAttached() || !Handle())
		return false;

	state = jack_transport_query(Handle(), &position);
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
	if (!IsAttached() || !Handle())
		return false;

	jack_transport_start(Handle());
	return true;
}

bool JackClient::StopTransport()
{
	if (!IsAttached() || !Handle())
		return false;

	jack_transport_stop(Handle());
	return true;
}

bool JackClient::LocateTransport(unsigned long frame)
{
	return IsAttached() && Handle() && jack_transport_locate(Handle(), (jack_nframes_t)frame) == 0;
}


bool JackClient::Attach(const string &clientName, const AudioClientOptions &options)
{
	if (WaitingForSession())
	{
		if (!m_Session->NativeReady())
		{
			if (!__atomic_load_n(&m_Session->prepared, __ATOMIC_ACQUIRE)) return false;
		}
		else if (ResizePortGroup(options.InChannels, options.OutChannels))
			return m_PortsReady = true;
	}
	Detach();
	if (options.InChannels > MAX_INPUTPORTS || options.OutChannels > MAX_OUTPUTPORTS ||
		(!options.InChannels && !options.OutChannels))
		return false;

	if (m_Owner)
		return AttachGroup(clientName, options.InChannels, options.OutChannels);

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
	if (m_Session)
	{
		if (!IsAttached() || !m_Session->clock.Start())
			return false;
		__atomic_store_n(&m_Session->running[m_Slot], this, __ATOMIC_SEQ_CST);
		return true;
	}

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


bool JackClient::GetCycleTiming(AudioCycleTiming &timing) const
{
	if (!m_ProcessFrames || !m_Timing.Valid) return false;

	timing = m_Timing;
	return true;
}

double JackClient::PortLatency(const JackPort &port, bool input) const
{
	if (!port.Port || !GetSampleRate()) return 0;

#ifdef HAVE_JACK_PORT_GET_LATENCY_RANGE
	jack_latency_range_t range;
	jack_port_get_latency_range(port.Port, input ? JackCaptureLatency : JackPlaybackLatency, &range);
	return double(range.max) / GetSampleRate();
#else
	return double(jack_port_get_total_latency(Handle(), port.Port)) / GetSampleRate();
#endif
}

double JackClient::MaximumLatency(const PortMap &ports, bool input) const
{
	double latency = 0;
	for (PortMap::const_iterator i = ports.begin(); i != ports.end(); ++i)
		latency = std::max(latency, PortLatency(i->second, input));

	return latency;
}

double JackClient::GetInputLatency() const
{
	return MaximumLatency(m_InputPortMap, true);
}

double JackClient::GetOutputLatency() const
{
	return MaximumLatency(m_OutputPortMap, false);
}

double JackClient::GetChannelTime(bool input, unsigned channel) const
{
	const PortMap &ports = input ? m_InputPortMap : m_OutputPortMap;
	PortMap::const_iterator i = ports.begin();
	while (channel && i != ports.end()) { --channel; ++i; }

	if (!m_ProcessFrames || i == ports.end()) return 0;

	const double latency = PortLatency(i->second, input);
	return m_CycleTime + (input ? -latency : latency);
}
