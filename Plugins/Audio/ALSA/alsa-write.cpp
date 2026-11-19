// Exercise a write that underruns after a successful ALSA readiness check.
#include "AlsaClient.h"
#include "AudioStream.h"
#include <cassert>
#include <cerrno>

using namespace Spiral::Core;

namespace
{
	const unsigned Period = 64;
	snd_pcm_sframes_t result;
	unsigned writes;
	bool invalidateParams;

	void WaitAt(AlsaClient &client, uint64_t frame)
	{
		AudioCycleTiming timing;
		assert(client.WaitForCycle(10) == 1);
		assert(client.GetCycleTiming(timing) && timing.Frame == frame);
	}
}

extern "C" snd_pcm_sframes_t scripted_write(snd_pcm_t *pcm, const void *, snd_pcm_uframes_t frames)
{
	assert(frames == Period);
	++writes;
	if (result == -EPIPE)
	{
		assert(snd_pcm_drop(pcm) == 0);
		if (invalidateParams) assert(snd_pcm_hw_free(pcm) == 0);
	}
	return result;
}

int main()
{
	AlsaClient client;
	AudioClientOptions options;
	options.BufferSize = Period;
	assert(client.Attach("null", options));
	AudioStream stream;
	assert(stream.Configure(&client, 0, 2, Period, options.Samplerate));

	WaitAt(client, 0);
	result = Period;
	assert(stream.Transfer(Period));
	WaitAt(client, Period);
	result = -EPIPE;
	const unsigned errors = stream.Errors();
	assert(stream.Transfer(Period) && !stream.Failed());
	assert(writes == 2 && stream.Errors() == errors + 1);

	AudioCycleTiming before, after;
	assert(stream.Timing(before));
	assert(!client.GetCycleTiming(after) && !client.RecoverPlaybackWrite());
	WaitAt(client, Period);
	result = Period;
	assert(stream.Transfer(Period) && stream.Timing(after));
	assert(!before.PlaybackRestart && after.PlaybackRestart);
	assert(after.Epoch == before.Epoch + 1 && after.Frame == before.Frame + Period);
	WaitAt(client, 2 * Period);
	assert(stream.Transfer(Period) && stream.Timing(before));
	assert(before.PlaybackRestart && before.Epoch == after.Epoch && writes == 4);

	// Preserve the accepted prefix of a short write. Retrying it would
	// duplicate frames. An unrelated device error must also remain fatal.
	result = Period / 2;
	assert(!stream.Transfer(Period) && stream.Failed());
	WaitAt(client, 3 * Period + Period / 2);
	assert(!client.RecoverPlaybackWrite());
	float samples[2 * Period] = {0};
	result = -ENODEV;
	assert(!client.Write(samples, Period) && !client.RecoverPlaybackWrite());
	WaitAt(client, 3 * Period + Period / 2);

	invalidateParams = true;
	result = -EPIPE;
	assert(!client.Write(samples, Period) && !client.RecoverPlaybackWrite());
	assert(!client.RecoverPlaybackWrite());
	return 0;
}
