// SPDX-License-Identifier: GPL-2.0-or-later
// Opt-in acoustic/cabled loopback. Two backends emit alternating quiet chirps.
#include "AudioBackend.h"
#include "AudioTimeline.h"
#include "JackClient.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
using namespace spiralcore;

struct RunOptions
{
	const char *InputBackend;
	const char *InputDevice;
	const char *OutputDevice[2];
	unsigned Pairs;

	RunOptions():
	    InputBackend("coreaudio"),
	    InputDevice("default"),
	    Pairs(8)
	{
		OutputDevice[0] = OutputDevice[1] = "default";
	}
};

struct Endpoint
{
	std::string Backend;
	AudioClient *Client;
	AudioStream Stream;

	Endpoint():
	    Client(NULL)
	{
	}

	~Endpoint()
	{
		if (Client)
		{
			Client->Detach();
			Stream.Stop();
			AudioBackendRegistry::Get()->Destroy(Backend, Client);
		}
	}

	bool Open(const std::string &backend, const std::string &device, unsigned inputs, unsigned outputs)
	{
		Backend = backend;
		Client = AudioBackendRegistry::Get()->Create(Backend);
		AudioClientOptions options;
		options.Samplerate = 48000;
		options.BufferSize = 256;
		options.InChannels = inputs;
		options.OutChannels = outputs;
		return Client && Client->Attach(device, options);
	}
};

struct Capture
{
	Endpoint Input;
	std::vector<float> Samples, Scratch;
	std::vector<double> Times;
	unsigned Written;
	unsigned Errors;

	Capture():
	    Written(0),
	    Errors(0)
	{
	}

	~Capture()
	{
		if (Input.Client)
			Input.Client->Detach();
	}

	static void Run(void *context, unsigned frames)
	{
		Capture &capture = *static_cast<Capture *>(context);
		if (!frames)
			return;

		AudioCycleTiming timing;
		if (frames > capture.Scratch.size() || !capture.Input.Client->GetCycleTiming(timing) ||
			!capture.Input.Client->Read(&capture.Scratch[0], frames))
		{
			++capture.Errors;
			return;
		}

		const unsigned count = std::min(frames, unsigned(capture.Samples.size()) - capture.Written);
		const double step = timing.Step > 0 ? timing.Step : 1.0 / capture.Input.Client->GetSampleRate();
		const double first = capture.Input.Client->GetChannelTime(true, 0);
		for (unsigned n = 0; n < count; ++n)
		{
			capture.Samples[capture.Written + n] = capture.Scratch[n];
			capture.Times[capture.Written + n] = first + n * step;
		}

		capture.Written += count;
	}
};

static float Chirp(double time)
{
	if (time < 0)
		return 0;

	time -= std::floor(time);
	if (time >= .12)
		return 0;

	const double envelope = std::sin(3.141592653589793 * time / .12);
	return float(.015 * envelope * envelope * std::sin(6.283185307179586 * (700 * time + 9000 * time * time)));
}

static bool Run(const char *firstBackend, const char *secondBackend, const char *prefix, const RunOptions &options)
{
	Capture capture;
	if (!capture.Input.Open(options.InputBackend, options.InputDevice, 1, 0))
	{
		std::fprintf(stderr, "Cannot open loopback capture device %s\n", options.InputDevice);
		return false;
	}

	const unsigned rate = capture.Input.Client->GetSampleRate();
	capture.Samples.resize(size_t(rate) * (options.Pairs + 6));
	capture.Times.resize(capture.Samples.size());
	capture.Scratch.resize(65536);
	capture.Input.Client->SetCallback(Capture::Run, &capture);
	if (!capture.Input.Client->Start())
		return false;

	Endpoint outputs[2];
	const char *backends[2] = {firstBackend, secondBackend};
	AudioTimeline timeline;
	for (unsigned n = 0; n < 2; ++n)
	{
		char name[40];
		std::snprintf(name, sizeof(name), "ssmph%ld-%u", long(getpid()), n);
		const bool jack = std::string(backends[n]) == "jack";
		if (!outputs[n].Open(backends[n], jack ? name : options.OutputDevice[n], 0, 2))
			return false;

		if (!outputs[n].Stream.Configure(outputs[n].Client, 0, 2, 256, 48000))
			return false;

		outputs[n].Client->SetCallback(AudioStream::Callback, &outputs[n].Stream);
		timeline.Register(&outputs[n].Stream);
		if (!outputs[n].Stream.Start())
			return false;

		if (jack)
		{
			JackClient *client = static_cast<JackClient *>(outputs[n].Client);
			client->ConnectOutput(0, "system:playback_1");
			client->ConnectOutput(1, "system:playback_2");
		}

		std::fprintf(stderr, "%s: rate=%lu period=%lu reported_output_ms=%.3f\n", backends[n],
			     outputs[n].Client->GetSampleRate(), outputs[n].Client->GetBufferSize(),
			     1000 * outputs[n].Client->GetOutputLatency());
	}

	const double epoch = AudioMonotonicTime() + 1;
	std::vector<float> signal(256 * 2);
	while (AudioMonotonicTime() < epoch + options.Pairs + 2)
	{
		if (timeline.Prepare(256, 48000, AudioMonotonicTime()))
		{
			const AudioStamp &stamp = timeline.PlaybackStamp();
			for (unsigned output = 0; output < 2; ++output)
			{
				for (unsigned n = 0; n < 256; ++n)
					signal[2 * n] = signal[2 * n + 1] = Chirp(stamp.Time + n * stamp.Step - epoch - output * .5);

				outputs[output].Stream.Playback(&signal[0], 256, stamp);
			}

			timeline.Commit(256);
		}

		usleep(timeline.SleepMicroseconds());
	}

	for (unsigned n = 0; n < 2; ++n)
	{
		outputs[n].Client->Detach();
		outputs[n].Stream.Stop();
		timeline.Unregister(&outputs[n].Stream);
		std::fprintf(stderr, "%s stream_errors=%u\n", backends[n], outputs[n].Stream.Errors());
	}

	capture.Input.Client->Detach();
	const std::string stem(prefix);
	FILE *samples = std::fopen((stem + ".f32").c_str(), "wb");
	FILE *times = std::fopen((stem + ".f64").c_str(), "wb");
	FILE *info = std::fopen((stem + ".txt").c_str(), "w");
	bool ok = samples && times && info && capture.Written > size_t(rate) * options.Pairs && !capture.Errors;
	if (samples)
	{
		ok = std::fwrite(&capture.Samples[0], sizeof(float), capture.Written, samples) == capture.Written && ok;
		std::fclose(samples);
	}

	if (times)
	{
		ok = std::fwrite(&capture.Times[0], sizeof(double), capture.Written, times) == capture.Written && ok;
		std::fclose(times);
	}

	if (info)
	{
		std::fprintf(info, "epoch=%.9f\nrate=%u\nframes=%u\nerrors=%u\nfirst=%s\nsecond=%s\ncapture=%s\n"
				   "pairs=%u\ncapture_device=%s\nfirst_device=%s\nsecond_device=%s\n",
			     epoch, rate, capture.Written, capture.Errors, firstBackend, secondBackend, options.InputBackend,
			     options.Pairs, options.InputDevice, options.OutputDevice[0], options.OutputDevice[1]);
		std::fclose(info);
	}

	return ok;
}

static bool ParseOptions(int argc, char **argv, RunOptions &options)
{
	if (argc < 5)
		return false;

	int index = 5;
	if (index < argc && argv[index][0] != '-')
		options.InputBackend = argv[index++];

	while (index < argc)
	{
		const std::string option(argv[index++]);
		if (index == argc)
			return false;

		const char *value = argv[index++];
		if (option == "--pairs")
		{
			char *end = NULL;
			const long count = std::strtol(value, &end, 10);
			if (end == value || *end || count < 8 || count > 300)
				return false;

			options.Pairs = unsigned(count);
		}
		else if (option == "--capture-device")
			options.InputDevice = value;
		else if (option == "--first-device")
			options.OutputDevice[0] = value;
		else if (option == "--second-device")
			options.OutputDevice[1] = value;
		else
			return false;
	}

	// JACK uses generated client names and the server's physical output ports.
	if ((std::string(argv[2]) == "jack" && std::string(options.OutputDevice[0]) != "default") ||
		(std::string(argv[3]) == "jack" && std::string(options.OutputDevice[1]) != "default"))
		return false;

	return true;
}

int main(int argc, char **argv)
{
	RunOptions options;
	if (!ParseOptions(argc, argv, options))
	{
		std::fprintf(stderr, "Usage: %s module-root backend-a backend-b result-prefix [capture-backend]\n"
				     "  [--pairs 8..300] [--capture-device ID] [--first-device ID] [--second-device ID]\n",
			     argv[0]);
		return 2;
	}

	alarm(options.Pairs + 20);
	AudioBackendRegistry::Get()->LoadModules(argv[1]);
	const bool ok = Run(argv[2], argv[3], argv[4], options);
	AudioBackendRegistry::PackUpAndGoHome();
	PluginLoader::PackUpAndGoHome();
	return ok ? 0 : 1;
}
