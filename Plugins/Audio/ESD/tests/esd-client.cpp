// SPDX-License-Identifier: GPL-2.0-or-later
#include "ESDClient.h"
#include <esd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <cmath>
using namespace spiralcore;
static int playback = -1, capture = -1;

extern "C" int esd_play_stream(esd_format_t, int, const char *, const char *)
{
	int fds[2];
	assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
	playback = fds[1];
	return fds[0];
}

extern "C" int esd_record_stream(esd_format_t, int, const char *, const char *)
{
	int fds[2];
	assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
	capture = fds[1];
	return fds[0];
}

int main()
{
	AudioClientOptions options;
	options.BufferSize = 64;
	options.Samplerate = 48000;
	options.InChannels = 2;
	options.OutChannels = 1;
	ESDClient client;
	assert(client.Attach("default", options));
	short input[128];
	for (unsigned n = 0; n < 128; ++n)
		input[n] = n % 2 ? -8192 : 8192;
	assert(write(capture, input, 100) == 100);
	assert(client.WaitForCycle(1) == 0); // Incomplete capture is retained.
	assert(write(capture, reinterpret_cast<char *>(input) + 100, sizeof(input) - 100) == sizeof(input) - 100);
	assert(client.WaitForCycle(1) == 1);
	float received[128], sent[64];
	assert(client.Read(received, 64));
	for (unsigned n = 0; n < 128; ++n)
		assert(received[n] == (n % 2 ? -.25f : .25f));
	for (unsigned n = 0; n < 64; ++n)
		sent[n] = .5f;
	assert(client.Write(sent, 64));
	short output[64];
	assert(read(playback, output, sizeof(output)) == sizeof(output));
	for (unsigned n = 0; n < 64; ++n)
		assert(std::abs(output[n] - 16383) <= 1);
	AudioCycleTiming timing;
	assert(client.GetCycleTiming(timing) && timing.Estimated && timing.Valid);
	const double before = AudioMonotonicTime();
	assert(client.WaitForCycle(1) == 0);
	assert(AudioMonotonicTime() - before < .05);
	close(capture);
	close(playback);
	assert(client.WaitForCycle(1) == -1);
	client.Detach();
	client.Detach();
	assert(!client.IsAttached());
	options.InChannels = 3;
	assert(!client.Attach("default", options));
	puts("ESD partial capture, PCM conversion, bounded wait and disconnect PASS");
}
