// ALSA's null PCM is always ready. Script the readiness calls so a complete
// worker cycle, including late and partial wakes, can be exercised on it.
#include "AlsaClient.h"
#include <cassert>
#include <cerrno>

using namespace Spiral::Core;

namespace
{
	enum Operation { Avail, Wait };
	const snd_pcm_stream_t Playback = SND_PCM_STREAM_PLAYBACK;
	const snd_pcm_stream_t Capture = SND_PCM_STREAM_CAPTURE;

	struct Reply
	{
		Operation Call;
		snd_pcm_stream_t Direction;
		int Result;
	};

	const Reply *nextReply;
	const Reply *endReplies;
	unsigned expectedTimeout;
	snd_pcm_t *recovered;

	int ReplyTo(Operation call, snd_pcm_t *pcm)
	{
		assert(nextReply != endReplies);
		const Reply &reply = *nextReply++;
		assert(reply.Call == call && reply.Direction == snd_pcm_stream(pcm));

		if (reply.Result == -EPIPE || reply.Result == -ESTRPIPE)
		{
			assert(snd_pcm_drop(pcm) == 0);
			recovered = pcm;
		}
		return reply.Result;
	}

	template <unsigned N>
	void CheckCycle(AlsaClient &client, const Reply (&replies)[N], unsigned milliseconds, int expected)
	{
		nextReply = replies;
		endReplies = replies + N;
		expectedTimeout = milliseconds;
		recovered = NULL;

		assert(client.WaitForCycle(milliseconds) == expected);
		assert(nextReply == endReplies);
	}
}

extern "C" snd_pcm_sframes_t scripted_avail(snd_pcm_t *pcm)
{
	return ReplyTo(Avail, pcm);
}

extern "C" int scripted_wait(snd_pcm_t *pcm, int milliseconds)
{
	assert(milliseconds == int(expectedTimeout));
	return ReplyTo(Wait, pcm);
}

int main()
{
	AlsaClient client;
	AudioClientOptions options;
	options.BufferSize = 64;
	options.OutChannels = 2;
	options.InChannels = 2;
	assert(client.Attach("null", options));

	// Playback was ready before waiting on capture. Recheck both directions
	// and publish timing from the same call that received the wake.
	const Reply duplex[] = {
		{Avail, Playback, 128}, {Avail, Capture, 0}, {Wait, Capture, 1},
		{Avail, Playback, 64}, {Avail, Capture, 64}
	};
	CheckCycle(client, duplex, 7, 1);
	AudioCycleTiming timing;
	assert(client.GetCycleTiming(timing) && timing.Valid && timing.Frames == 64);

	const Reply partial[] = {{Avail, Playback, 0}, {Wait, Playback, 1}, {Avail, Playback, 63}};
	CheckCycle(client, partial, 7, 0);
	const Reply otherSideLate[] = {
		{Avail, Playback, 0}, {Wait, Playback, 1}, {Avail, Playback, 64}, {Avail, Capture, 0}
	};
	CheckCycle(client, otherSideLate, 7, 0);
	const Reply poll[] = {{Avail, Playback, 0}};
	CheckCycle(client, poll, 0, 0);

	const Reply timeout[] = {{Avail, Playback, 0}, {Wait, Playback, 0}};
	CheckCycle(client, timeout, 7, 0);
	const Reply interrupted[] = {{Avail, Playback, 0}, {Wait, Playback, -EINTR}};
	CheckCycle(client, interrupted, 7, 0);
	const Reply again[] = {{Avail, Playback, 0}, {Wait, Playback, -EAGAIN}};
	CheckCycle(client, again, 7, 0);
	const Reply disconnected[] = {{Avail, Playback, 0}, {Wait, Playback, -ENODEV}};
	CheckCycle(client, disconnected, 7, -1);

	const Reply underrun[] = {{Avail, Playback, 0}, {Wait, Playback, -EPIPE}};
	CheckCycle(client, underrun, 7, 0);
	assert(snd_pcm_state(recovered) == SND_PCM_STATE_PREPARED);
	const Reply suspended[] = {{Avail, Playback, 64}, {Avail, Capture, 0}, {Wait, Capture, -ESTRPIPE}};
	CheckCycle(client, suspended, 7, 0);
	assert(snd_pcm_state(recovered) == SND_PCM_STATE_RUNNING);
	const Reply overrun[] = {{Avail, Playback, 64}, {Avail, Capture, -EPIPE}};
	CheckCycle(client, overrun, 7, 0);
	assert(snd_pcm_state(recovered) == SND_PCM_STATE_RUNNING);

	const Reply ready[] = {{Avail, Playback, 64}, {Avail, Capture, 64}};
	CheckCycle(client, ready, 0, 1);
	return 0;
}
