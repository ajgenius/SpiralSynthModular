// SPDX-License-Identifier: GPL-2.0-or-later
// Opt-in live JACK test. All connections stay between these temporary clients.
#include "AudioTimeline.h"
#include "JackClient.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
using namespace spiralcore;

struct Endpoint
{
	JackClient Client;
	AudioStream Stream;

	~Endpoint()
	{
		Client.Detach();
		Stream.Stop();
	}
};

struct Capture
{
	JackClient Client;
	std::vector<float> Scratch, Samples;
	unsigned Written;
	volatile unsigned Errors;
	volatile int Enabled;

	Capture():
	    Written(0),
	    Errors(0),
	    Enabled(0)
	{
	}

	~Capture()
	{
		Client.Detach();
	}

	static void Run(void *context, unsigned frames)
	{
		Capture &capture = *static_cast<Capture *>(context);
		if (!frames || !__sync_fetch_and_add(&capture.Enabled, 0))
			return;

		if (frames * 2 > capture.Scratch.size() || !capture.Client.Read(&capture.Scratch[0], frames))
		{
			__sync_fetch_and_add(&capture.Errors, 1);
			return;
		}

		const unsigned count = std::min(unsigned(capture.Samples.size()) - capture.Written, frames * 2);
		std::copy(capture.Scratch.begin(), capture.Scratch.begin() + count, capture.Samples.begin() + capture.Written);
		capture.Written += count;
	}
};

static bool Run(unsigned graphFrames, unsigned graphRate)
{
	char prefix[80];
	std::snprintf(prefix, sizeof(prefix), "ssmlb%ld-%u", long(getpid()), graphFrames);
	Endpoint outputs[2];
	Capture capture;
	AudioClientOptions options;
	options.InChannels = 2;
	options.OutChannels = 0;
	if (!capture.Client.Attach(std::string(prefix) + "-in", options))
		return false;

	const unsigned rate = capture.Client.GetSampleRate();
	const unsigned period = capture.Client.GetBufferSize();
	capture.Scratch.resize(capture.Client.GetBufferSize() * 2);
	capture.Samples.resize(rate * 3 * 2);
	capture.Client.SetCallback(Capture::Run, &capture);
	if (!capture.Client.Start())
		return false;

	AudioTimeline timeline;
	options.InChannels = 0;
	options.OutChannels = 1;
	for (unsigned n = 0; n < 2; ++n)
	{
		const std::string name = std::string(prefix) + (n ? "-b" : "-a");
		if (!outputs[n].Client.Attach(name, options))
			return false;

		if (!outputs[n].Stream.Configure(&outputs[n].Client, 0, 1, graphFrames, graphRate))
			return false;

		outputs[n].Client.SetCallback(AudioStream::Callback, &outputs[n].Stream);
		timeline.Register(&outputs[n].Stream);
		if (!outputs[n].Stream.Start())
			return false;

		capture.Client.ConnectInput(n, name + ":Out0");
	}

	std::vector<float> signal(graphFrames);
	const double start = AudioMonotonicTime();
	unsigned blocks = 0;
	while (AudioMonotonicTime() < start + 4.5)
	{
		const double now = AudioMonotonicTime();
		if (timeline.Prepare(graphFrames, graphRate, now))
		{
			const AudioStamp &stamp = timeline.PlaybackStamp();
			for (unsigned n = 0; n < graphFrames; ++n)
			{
				const double time = stamp.Time + n * stamp.Step - start;
				signal[n] = .01 * (std::sin(time * 997 * 6.283185307179586) +
					std::sin(time * 1733 * 6.283185307179586));
			}

			for (unsigned n = 0; n < 2; ++n)
				outputs[n].Stream.Playback(&signal[0], graphFrames, stamp);

			timeline.Commit(graphFrames);
			++blocks;
		}

		if (now > start + 1)
			__sync_lock_test_and_set(&capture.Enabled, 1);

		usleep(timeline.SleepMicroseconds());
	}

	capture.Client.Detach();
	for (unsigned n = 0; n < 2; ++n)
	{
		outputs[n].Client.Detach();
		outputs[n].Stream.Stop();
		timeline.Unregister(&outputs[n].Stream);
	}

	double energy = 0, difference = 0, peak = 0;
	unsigned missing = 0;
	for (unsigned n = 0; n + 1 < capture.Written; n += 2)
	{
		const double a = capture.Samples[n], b = capture.Samples[n + 1];
		energy += a * a + b * b;
		difference += (a - b) * (a - b);
		peak = std::max(peak, std::fabs(a - b));
		if ((a == 0) != (b == 0))
			++missing;
	}

	const unsigned frames = capture.Written / 2;
	const double rms = frames ? std::sqrt(energy / (2 * frames)) : 0;
	const double error = frames ? std::sqrt(difference / frames) : 1;
	std::printf("JACK alignment graph=%u/%u native=%u/%u captured=%u rendered=%u "
		    "rms=%.7f difference_rms=%.9f difference_peak=%.9f unequal_silence=%u read_errors=%u\n",
		    graphRate, graphFrames, rate, period, frames, blocks,
		    rms, error, peak, missing, capture.Errors);
	return frames >= rate * 2 && rms > .005 && error < .00001 && peak < .0001 && !missing && !capture.Errors;
}

int main()
{
	alarm(30);
	return Run(256, 48000) && Run(511, 44100) && Run(2048, 48000) ? 0 : 1;
}
