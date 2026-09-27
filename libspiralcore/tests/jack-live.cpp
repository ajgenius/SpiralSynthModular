// Live-server smoke test. Routes only between its own temporary clients.
#include "JackClient.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <vector>

struct Endpoint
{
	spiralcore::JackClient Client;
	std::vector<float> Buffer;
	volatile unsigned Cycles, Matches, Errors;
	bool Capture;
	Endpoint(bool capture) : Cycles(0), Matches(0), Errors(0), Capture(capture) {}

	static void Run(void *context, unsigned frames)
	{
		Endpoint &endpoint = *static_cast<Endpoint *>(context);
		if (!frames) return;

		if (frames * 2 > endpoint.Buffer.size())
		{
			__sync_fetch_and_add(&endpoint.Errors, 1);
			return;
		}

		bool ok;
		if (endpoint.Capture)
		{
			ok = endpoint.Client.Read(&endpoint.Buffer[0], frames);
			bool matches = ok;
			for (unsigned n = 0; n < frames; ++n)
				matches = matches && endpoint.Buffer[2*n] == 0.25f && endpoint.Buffer[2*n+1] == -0.5f;

			if (matches) __sync_fetch_and_add(&endpoint.Matches, 1);

		}

		else
		{
			for (unsigned n = 0; n < frames; ++n)
			{
				endpoint.Buffer[2*n] = 0.25f;
				endpoint.Buffer[2*n+1] = -0.5f;
			}

			ok = endpoint.Client.Write(&endpoint.Buffer[0], frames);
		}

		if (!ok) __sync_fetch_and_add(&endpoint.Errors, 1);

		__sync_fetch_and_add(&endpoint.Cycles, 1);
	}

};

int main()
{
	char source[64], sink[64];
	snprintf(source, sizeof(source), "SSM-test-source-%ld", long(getpid()));
	snprintf(sink, sizeof(sink), "SSM-test-sink-%ld", long(getpid()));
	for (unsigned attempt = 0; attempt < 5; ++attempt)
	{
		Endpoint writer(false), reader(true);
		spiralcore::AudioClientOptions out, in;
		out.InChannels = 0; out.OutChannels = 2;
		in.InChannels = 2; in.OutChannels = 0;
		if (!writer.Client.Attach(source, out) || !reader.Client.Attach(sink, in)) return 1;

		writer.Buffer.resize(writer.Client.GetBufferSize() * 2);
		reader.Buffer.resize(reader.Client.GetBufferSize() * 2);
		writer.Client.SetCallback(Endpoint::Run, &writer);
		reader.Client.SetCallback(Endpoint::Run, &reader);
		if (!writer.Client.Start() || !reader.Client.Start()) return 2;

		reader.Client.ConnectInput(0, std::string(source) + ":Out0");
		reader.Client.ConnectInput(1, std::string(source) + ":Out1");
		usleep(300000);
		jack_transport_state_t state;
		jack_position_t position;
		if (!reader.Client.QueryTransport(state, position)) return 3;

		printf("cycle %u: %lu Hz / %lu frames, transport=%d\n", attempt + 1,
			reader.Client.GetSampleRate(), reader.Client.GetBufferSize(), int(state));
		reader.Client.Detach();
		writer.Client.Detach();
		printf("  source=%u capture=%u matched=%u errors=%u\n",
			writer.Cycles, reader.Cycles, reader.Matches, writer.Errors + reader.Errors);
		if (reader.Matches < 3 || reader.Errors || writer.Errors) return 4;

	}

	puts("Live JACK stereo and repeated client teardown PASS");
	return 0;
}
