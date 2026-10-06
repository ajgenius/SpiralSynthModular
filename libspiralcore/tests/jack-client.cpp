// Exercise the real JackClient against a deterministic JACK API, without a server.
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "JackClient.h"
#include <cmath>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

struct _jack_port
{
	std::string Name;

	std::string Connection;

	float Buffer[32];
	_jack_port() { memset(Buffer, 0, sizeof(Buffer)); }

};

struct _jack_client
{
	std::string Name;
	JackProcessCallback Process;
	JackSampleRateCallback Rate;
	JackBufferSizeCallback Size;
	JackShutdownCallback Shutdown;
	void *ProcessContext;
	void *RateContext;
	void *SizeContext;
	void *ShutdownContext;
	bool Active;

	std::vector<jack_port_t *> Ports;
	_jack_client() : Process(NULL), Rate(NULL), Size(NULL), Shutdown(NULL),
		ProcessContext(NULL), RateContext(NULL), SizeContext(NULL), ShutdownContext(NULL), Active(false) {}

};

static std::map<std::string, jack_client_t *> clients;
static jack_transport_state_t transportState = JackTransportStopped;
static jack_nframes_t transportFrame = 0;

static jack_nframes_t cycleFrame = 48000;
static double cycleTimeOffset = 0;
static bool preciseCycle = false;
static bool failOpen = false;
static bool failActivate = false;
static bool failRegister = false;
static bool failCallback = false;

extern "C"
{
int jack_get_cycle_times(const jack_client_t *, jack_nframes_t *frame, jack_time_t *start, jack_time_t *end, float *estimate)
{
	if (!preciseCycle) return -1;

	*frame = cycleFrame;
	*start = 10000000;
	*end = *start + 84;
	*estimate = 999;
	return 0;
}
jack_time_t jack_get_time() { return jack_time_t(spiralcore::AudioMonotonicTime() * 1e6); }
jack_nframes_t jack_last_frame_time(const jack_client_t *) { return cycleFrame; }
jack_time_t jack_frames_to_time(const jack_client_t *, jack_nframes_t frame) { return jack_time_t((cycleTimeOffset + double(frame) / 48000) * 1e6); }
#ifdef HAVE_JACK_PORT_GET_LATENCY_RANGE
void jack_port_get_latency_range(jack_port_t *, jack_latency_callback_mode_t, jack_latency_range_t *range)
{
	range->min = range->max = 4;
}
#endif
jack_nframes_t jack_port_get_total_latency(jack_client_t *, jack_port_t *) { return 4; }

jack_transport_state_t jack_transport_query(const jack_client_t *, jack_position_t *position)
{
	memset(position, 0, sizeof(*position));
	position->frame = transportFrame;
	return transportState;
}

void jack_transport_start(jack_client_t *) { transportState = JackTransportRolling; }

void jack_transport_stop(jack_client_t *) { transportState = JackTransportStopped; }

int jack_transport_locate(jack_client_t *, jack_nframes_t frame)
{
	transportFrame = frame;
	return 0;
}

jack_client_t *jack_client_new(const char *name)
{
	if (failOpen)
	{
		// libjack may destroy remaining stale clients on a failed reopen.
		assert(clients.empty());
		return NULL;
	}

	jack_client_t *client = new jack_client_t;
	client->Name = name;
	clients[name] = client;
	return client;
}

jack_client_t *jack_client_open(const char *name, jack_options_t options, jack_status_t *, ...)
{
	assert(options & JackNoStartServer);
	return jack_client_new(name);
}

int jack_client_close(jack_client_t *client)
{
	clients.erase(client->Name);
	for (size_t n = 0; n < client->Ports.size(); ++n) delete client->Ports[n];

	delete client;
	return 0;
}

jack_nframes_t jack_get_buffer_size(jack_client_t *) { return 4; }

jack_nframes_t jack_get_sample_rate(jack_client_t *) { return 48000; }

int jack_set_process_callback(jack_client_t *c, JackProcessCallback f, void *p)
{
	c->Process = f; c->ProcessContext = p;
	return failCallback ? 1 : 0;
}

int jack_set_sample_rate_callback(jack_client_t *c, JackSampleRateCallback f, void *p)
{
	c->Rate = f; c->RateContext = p;
	return 0;
}

int jack_set_buffer_size_callback(jack_client_t *c, JackBufferSizeCallback f, void *p)
{
	c->Size = f; c->SizeContext = p;
	return 0;
}

void jack_on_shutdown(jack_client_t *c, JackShutdownCallback f, void *p)
{
	c->Shutdown = f; c->ShutdownContext = p;
}

int jack_activate(jack_client_t *c) { c->Active = !failActivate; return failActivate ? 1 : 0; }

int jack_deactivate(jack_client_t *c) { c->Active = false; return 0; }

jack_port_t *jack_port_register(jack_client_t *c, const char *name, const char *, unsigned long, unsigned long)
{
	assert(!c->Active);
	if (failRegister) return NULL;

	jack_port_t *port = new jack_port_t;
	port->Name = c->Name + ":" + name;
	c->Ports.push_back(port);
	return port;
}

int jack_port_unregister(jack_client_t *c, jack_port_t *port)
{
	assert(!c->Active);
	for (std::vector<jack_port_t *>::iterator i = c->Ports.begin(); i != c->Ports.end(); ++i)
	{
		if (*i == port) { c->Ports.erase(i); break; }

	}

	delete port;
	return 0;
}

void *jack_port_get_buffer(jack_port_t *port, jack_nframes_t frames)
{
	assert(frames <= 32);
	return port->Buffer;
}

int jack_port_connected(const jack_port_t *port) { return !port->Connection.empty(); }

const char *jack_port_name(const jack_port_t *port) { return port->Name.c_str(); }

const char **jack_get_ports(jack_client_t *, const char *, const char *, unsigned long) { return NULL; }

const char **jack_port_get_all_connections(const jack_client_t *, const jack_port_t *port)
{
	if (port->Connection.empty()) return NULL;

	const char **names = static_cast<const char **>(malloc(2 * sizeof(char *)));
	names[0] = port->Connection.c_str(); names[1] = NULL;
	return names;
}

void jack_free(void *data) { free(data); }

int jack_port_disconnect(jack_client_t *, jack_port_t *port) { port->Connection.clear(); return 0; }

int jack_connect(jack_client_t *c, const char *source, const char *destination)
{
	for (size_t n = 0; n < c->Ports.size(); ++n)
	{
		jack_port_t *port = c->Ports[n];
		if (port->Name == source) port->Connection = destination;

		if (port->Name == destination) port->Connection = source;

	}

	return 0;
}

}

// The plugin period tests reuse this server without running the client suite.
#ifndef JACK_CLIENT_TEST_FIXTURE
struct Notifications
{
	unsigned int Frames;

	unsigned int Stops;
	Notifications() : Frames(0), Stops(0) {}

	static void Run(void *context, unsigned int frames)
	{
		Notifications *n = static_cast<Notifications *>(context);
		if (frames) n->Frames = frames;

		else ++n->Stops;
	}

};

struct AudioCycle
{
	spiralcore::AudioClient *Client;
	float Captured[8];
	spiralcore::AudioCycleTiming Timing;
	static void Run(void *context, unsigned int frames)
	{
		if (!frames)
			return;

		AudioCycle *cycle = static_cast<AudioCycle *>(context);
		assert(frames == 4);
		assert(cycle->Client->GetCycleTiming(cycle->Timing));
		assert(cycle->Timing.Frame >= cycleFrame);
		assert(cycle->Timing.OutputTime > cycle->Timing.InputTime);
		assert(cycle->Client->GetChannelTime(false, 0) == cycle->Timing.OutputTime);
		assert(cycle->Client->Read(cycle->Captured, frames));
		assert(cycle->Client->Write(cycle->Captured, frames));
	}

};

int main()
{
	using spiralcore::JackClient;
	Notifications first, second;
	JackClient a, b;
	a.SetCallback(Notifications::Run, &first);
	b.SetCallback(Notifications::Run, &second);
	assert(a.AddInputPort() == 0);
	assert(a.AddOutputPort() == 0);
	assert(b.AddOutputPort() == 0);
	assert(a.Attach("SSM0"));
	assert(b.Attach("SSM1"));
	jack_transport_state_t state;
	jack_position_t position;
	assert(a.QueryTransport(state, position) && state == JackTransportStopped);
	assert(a.StartTransport());
	assert(b.QueryTransport(state, position) && state == JackTransportRolling);
	assert(b.LocateTransport(12345));
	assert(a.QueryTransport(state, position) && position.frame == 12345);
	assert(b.StopTransport());
	assert(a.QueryTransport(state, position) && state == JackTransportStopped);
	assert(a.GetBufferSize() == 4 && a.GetSampleRate() == 48000);
	assert(a.GetInputName(0) == "In0" && a.GetOutputName(0) == "Out0");
	assert(a.GetInputName(42).empty());
	jack_client_t *native = clients["SSM0"];
	float input[6] = {-1, -1, -1, -1, 99, 99};

	float output[4] = {1, 2, 3, 4};

	a.SetInputBuf(0, input, 4);
	a.SetOutputBuf(0, output, 4);
	a.ConnectInput(0, "system:capture_1");
	a.ConnectOutput(0, "system:playback_1");
	assert(a.GetInputConnection(0) == "system:capture_1");
	assert(a.GetOutputConnection(0) == "system:playback_1");
	for (int n = 0; n < 4; ++n) native->Ports[0]->Buffer[n] = float(n + 5);

	native->Process(4, native->ProcessContext);
	assert(first.Frames == 4 && second.Frames == 0);
	assert(input[0] == 5 && input[3] == 8);
	assert(native->Ports[1]->Buffer[0] == 1 && native->Ports[1]->Buffer[3] == 4);

	// A server buffer change must not overrun patch buffers awaiting resize.
	native->Size(8, native->SizeContext);
	native->Rate(96000, native->RateContext);
	native->Process(8, native->ProcessContext);
	assert(a.GetBufferSize() == 8 && a.GetSampleRate() == 96000);
	assert(input[4] == 99 && input[5] == 99);
	for (int n = 0; n < 8; ++n) assert(native->Ports[1]->Buffer[n] == 0);

	for (int n = 0; n < 4; ++n) assert(input[n] == 0);

	a.DisconnectInput(0);
	a.DisconnectOutput(0);
	assert(a.GetInputConnection(0).empty() && a.GetOutputConnection(0).empty());
	std::vector<std::string> inputs, outputs;
	a.GetPortNames(inputs, outputs);
	assert(inputs.empty() && outputs.empty());
	assert(a.AddOutputPort(1) == 1);
	a.RemoveOutputPort(1);
	assert(a.AddOutputPort(1) == 1);
	assert(native->Active);

	// Shutdown reports zero frames, preserves port descriptors, and permits reconnect.
	native->Shutdown(native->ShutdownContext);
	assert(!a.IsAttached() && first.Stops == 1 && b.IsAttached());
	assert(a.Attach("SSM0"));
	assert(a.GetOutputName(1) == "Out1");
	a.Detach(); a.Detach();
	assert(first.Stops == 2 && b.IsAttached());
	b.Detach();
	assert(second.Stops == 1 && clients.empty());
	assert(!a.QueryTransport(state, position));
	assert(!a.StartTransport() && !a.StopTransport() && !a.LocateTransport(0));

	failOpen = true; assert(!a.Attach("fail")); failOpen = false;
	failRegister = true; assert(!a.Attach("fail")); failRegister = false;
	failActivate = true; assert(!a.Attach("fail")); failActivate = false;
	failCallback = true; assert(!a.Attach("fail")); failCallback = false;
	assert(!a.IsAttached() && clients.empty());
	assert(a.Attach("SSM0"));
	a.Detach();
	assert(clients.empty());
	// The shared AudioClient prepares before activation and exchanges stereo
	// only during a callback. Blocking backends retain their original API.
	spiralcore::AudioClient *audio = &a;
	spiralcore::AudioClientOptions options;
	options.InChannels = options.OutChannels = 2;
	AudioCycle cycle;
	cycle.Client = audio;
	audio->SetCallback(AudioCycle::Run, &cycle);
	assert(audio->IsCallbackDriven());
	assert(audio->Attach("stereo", options));
	native = clients["stereo"];
	assert(!native->Active && native->Ports.size() == 4);
	assert(audio->GetBufferSize() == 4 && audio->GetSampleRate() == 48000);
	assert(!audio->Read(cycle.Captured, 4) && !audio->Write(cycle.Captured, 4));
	for (unsigned int frame = 0; frame < 4; ++frame)
	{
		native->Ports[0]->Buffer[frame] = float(frame);
		native->Ports[1]->Buffer[frame] = float(frame + 10);
	}

	assert(audio->Start() && native->Active);
	native->Process(4, native->ProcessContext);
	for (unsigned int frame = 0; frame < 4; ++frame)
	{
		assert(cycle.Captured[2 * frame] == float(frame));
		assert(cycle.Captured[2 * frame + 1] == float(frame + 10));
		assert(native->Ports[2]->Buffer[frame] == float(frame));
		assert(native->Ports[3]->Buffer[frame] == float(frame + 10));
	}

#ifdef HAVE_JACK_GET_CYCLE_TIMES
	preciseCycle = true;
	native->Process(4, native->ProcessContext);
	assert(std::fabs(cycle.Timing.Step - 21e-6) < 1e-12);
	preciseCycle = false;
#endif
	cycleFrame = 0xfffffffcU;
	native->Process(4, native->ProcessContext);
	const uint64_t beforeWrap = cycle.Timing.Frame;
	cycleFrame = 0;
	native->Process(4, native->ProcessContext);
	assert(cycle.Timing.Frame == beforeWrap + 4);
	spiralcore::AudioCycleTiming outsideCallback;
	assert(!audio->GetCycleTiming(outsideCallback));
	audio->Detach();
	assert(!audio->Start());
	options.InChannels = 0;
	assert(audio->Attach("write", options));
	assert(clients["write"]->Ports.size() == 2);
	audio->Detach();
	options.InChannels = 2;
	options.OutChannels = 0;
	assert(audio->Attach("read", options));
	assert(clients["read"]->Ports.size() == 2);
	audio->Detach();
	assert(clients.empty());
	assert(a.Attach("restart-a") && b.Attach("restart-b"));
	clients["restart-a"]->Shutdown(clients["restart-a"]->ShutdownContext);
	clients["restart-b"]->Shutdown(clients["restart-b"]->ShutdownContext);
	failOpen=true;
	assert(!a.Attach("restart-a"));
	assert(clients.empty());
	b.Detach();
	failOpen=false;
	assert(a.Attach("restart-a") && b.Attach("restart-b"));
	a.Detach(); b.Detach();
	assert(JackClient::Get() == JackClient::Get());
	JackClient::PackUpAndGoHome();
	return 0;
}

#endif
