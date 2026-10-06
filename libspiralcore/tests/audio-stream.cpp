// Multiple physical clocks and channel latencies, without audio hardware.
#include "AudioStream.h"
#include "PresentationClock.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <unistd.h>
#include <new>
#include <cstdlib>
using namespace spiralcore;
static bool insideCallback = false;
void *operator new(std::size_t bytes) throw(std::bad_alloc)
{
	assert(!insideCallback);
	void *value = std::malloc(bytes ? bytes : 1);
	if (!value) throw std::bad_alloc();

	return value;
}
void *operator new[](std::size_t bytes) throw(std::bad_alloc) { return ::operator new(bytes); }
void operator delete(void *value) throw() { std::free(value); }
void operator delete[](void *value) throw() { std::free(value); }

static double Signal(double time) { return std::sin(time * 997 * 6.283185307179586); }

class Device : public AudioClient
{
public:
	unsigned Period;
	double Rate, ActualRate, Next, Latency;
	uint64_t Frame;
	AudioCycleTiming Cycle;
	AudioStream Stream;
	double Peak, SettledPeak;
	unsigned Checked;
	bool Muted;
	Device(unsigned period, double rate, double ppm, double phase, double latency) :
		Period(period), Rate(rate), ActualRate(rate * (1 + ppm / 1e6)), Next(10 + phase),
		Latency(latency), Frame(0), Peak(0), SettledPeak(0), Checked(0), Muted(false) {}
	bool Attach(const std::string &, const AudioClientOptions &) { return true; }
	void Detach() {}
	bool IsAttached() const { return true; }
	unsigned long GetBufferSize() const { return Period; }
	unsigned long GetSampleRate() const { return (unsigned long)Rate; }
	double GetInputLatency() const { return 0.01; }
	double GetOutputLatency() const { return Latency + 0.003; }
	bool GetCycleTiming(AudioCycleTiming &timing) const { timing = Cycle; return Cycle.Valid; }
	double GetChannelTime(bool input, unsigned channel) const
	{
		return input ? Cycle.InputTime - channel * 0.002 : Cycle.OutputTime + channel * 0.003;
	}
	bool Read(float *samples, unsigned frames)
	{
		for (unsigned n = 0; n < frames; ++n)
			for (unsigned c = 0; c < 2; ++c)
				samples[2*n+c] = Signal(GetChannelTime(true, c) + n / ActualRate);

		return true;
	}
	bool Write(const float *samples, unsigned frames)
	{
		if (Next > 10.25 && !Muted)
			for (unsigned n = 0; n < frames; ++n)
				for (unsigned c = 0; c < 2; ++c)
				{
					const double expected = Signal(GetChannelTime(false, c) + n / ActualRate);
					const double error = std::fabs(samples[2*n+c] - expected);
					Peak = std::max(Peak, error);
					if (Next > 12) SettledPeak = std::max(SettledPeak, error);
					++Checked;
				}

		return true;
	}
	void Tick()
	{
		Cycle.Valid = true;
		Cycle.Frame = Frame;
		Cycle.Frames = Period;
		Cycle.SampleRate = Rate;
		Cycle.CallbackTime = Next;
		Cycle.OutputTime = Next + Latency;
		Cycle.InputTime = Next - 0.01;
		insideCallback = true;
		assert(Stream.Transfer(Period));
		insideCallback = false;
		Frame += Period;
		Next += Period / ActualRate;
	}
};

static void Run(double engineRate, unsigned enginePeriod, double slaveRate, double ppm)
{
	Device a(128, 48000, 0, 0, 0.012);
	Device b(384, slaveRate, ppm, 0.0037, 0.028);
	assert(a.Stream.Configure(&a, 2, 2, enginePeriod, engineRate));
	assert(b.Stream.Configure(&b, 2, 2, enginePeriod, engineRate));
	PresentationClock clock;
	AudioCycleTiming master;
	AudioStamp stamp;
	std::vector<float> samples(enginePeriod * 2), captured(enginePeriod * 2);
	double now = 10, renderAt = 10, capturePeak = 0;
	unsigned rendered = 0;
	while (now < 20)
	{
		now = std::min(renderAt, std::min(a.Next, b.Next));
		if (a.Next <= now) a.Tick();

		if (b.Next <= now) b.Tick();

		if (renderAt <= now)
		{
			a.Stream.Timing(master);
			if (clock.Prepare(now, enginePeriod, engineRate, master.Valid ? &master : NULL, 0.065, stamp))
			{
				a.Stream.SetGeneration(stamp.Generation);
				b.Stream.SetGeneration(stamp.Generation);
				for (unsigned n = 0; n < enginePeriod; ++n)
					samples[n*2] = samples[n*2+1] = Signal(stamp.Time + n * stamp.Step);

				assert(a.Stream.Playback(&samples[0], enginePeriod, stamp));
				assert(b.Stream.Playback(&samples[0], enginePeriod, stamp));
				AudioStamp capture = stamp;
				capture.Time -= 0.1;
				b.Stream.Capture(&captured[0], enginePeriod, capture);
				if (now > 10.25)
					for (unsigned n = 0; n < enginePeriod; ++n)
						for (unsigned c = 0; c < 2; ++c)
							capturePeak = std::max(capturePeak, std::fabs(captured[2*n+c] - Signal(capture.Time + n * capture.Step)));

				clock.Commit(enginePeriod);
				++rendered;
			}

			renderAt = now + std::max(0.00001, clock.Wait());
		}

	}

	printf("graph %.0f/%u, slave %.0f %+.0fppm: output errors %.6f/%.6f, capture %.6f (%u periods)\n",
		engineRate, enginePeriod, slaveRate, ppm, a.Peak, b.Peak, capturePeak, rendered);
	assert(a.Checked > 100000 && b.Checked > 100000);
	assert(a.Peak < 0.015 && b.Peak < 0.015 && capturePeak < 0.015);
	assert(a.SettledPeak < 0.0002 && b.SettledPeak < 0.0002);
}

static void ClockTransitions()
{
	PresentationClock clock;
	AudioStamp stamp;
	assert(clock.Prepare(100, 480, 48000, NULL, 0.04, stamp));
	const unsigned initial = stamp.Generation;
	clock.Commit(480);
	clock.Reset();
	assert(clock.Prepare(101, 480, 48000, NULL, 0.04, stamp));
	assert(stamp.Frame == 480 && stamp.Generation != initial && stamp.Time >= 101.04);
	clock.Commit(480);
	// A long control stall discards old playout, keeping the graph frame monotonic.
	assert(clock.Prepare(105, 480, 48000, NULL, 0.04, stamp));
	assert(stamp.Frame == 960 && stamp.Time >= 105.04);
	clock.Commit(480);
	AudioCycleTiming native;
	native.Valid = true;
	native.SampleRate = 48000;
	native.Step = 1.0 / 48000;
	native.CallbackTime = native.OutputTime = 106;
	native.Frame = 48000;
	native.Epoch = 1;
	if (!clock.Prepare(106, 480, 48000, &native, 0.04, stamp))
		assert(clock.Prepare(106.0001, 480, 48000, &native, 0.04, stamp));

	const unsigned generation = stamp.Generation;
	clock.Commit(480);
	// A native xrun with a continuous callback counter still needs a new map.
	native.Epoch = 2;
	native.Frame += 480;
	native.CallbackTime = native.OutputTime = 106.02;
	if (!clock.Prepare(106.02, 480, 48000, &native, 0.04, stamp))
		assert(clock.Prepare(106.0201, 480, 48000, &native, 0.04, stamp));

	assert(stamp.Generation != generation && stamp.Frame == 1920);
}
static void StallRecovery()
{
	Device device(128, 48000, 0, 0, 0.01);
	assert(device.Stream.Configure(&device, 0, 2, 128, 48000));
	device.Stream.SetGeneration(1);
	AudioStamp stamp;
	stamp.Generation = 1;
	stamp.Step = 1.0 / 48000;
	stamp.Time = 10.06;
	float samples[256];
	unsigned queued = 0;
	for (; queued < 1000; ++queued)
	{
		for (unsigned n = 0; n < 128; ++n)
			samples[2*n] = samples[2*n+1] = Signal(stamp.Time + n * stamp.Step);

		if (!device.Stream.Playback(samples, 128, stamp)) break;

		stamp.Frame += 128;
		stamp.Time += 128 * stamp.Step;
	}

	assert(queued > 10 && queued < 1000);
	device.Next = 11;
	device.Muted = true;
	device.Tick();
	device.Stream.SetGeneration(++stamp.Generation);
	stamp.Time = device.Next + device.Latency;
	for (unsigned period = 0; period < 5; ++period)
	{
		for (unsigned n = 0; n < 128; ++n)
			samples[2*n] = samples[2*n+1] = Signal(stamp.Time + n * stamp.Step);

		assert(device.Stream.Playback(samples, 128, stamp));
		stamp.Frame += 128;
		stamp.Time += 128 * stamp.Step;
	}

	device.Muted = false;
	device.Tick();
	device.Tick();
	assert(device.Checked == 512 && device.Peak < 0.0002);
}

class StalledDevice : public Device
{
public:
	StalledDevice() : Device(128, 48000, 0, 0, 0.01) {}
	int WaitForCycle(unsigned milliseconds) { usleep(milliseconds * 1000); return 0; }
};
static void WorkerShutdown()
{
	StalledDevice device;
	assert(device.Stream.Configure(&device, 0, 2, 128, 48000));
	for (unsigned n = 0; n < 20; ++n)
	{
		assert(device.Stream.Start());
		assert(device.Stream.Start());
		usleep(1000);
		const double start = AudioMonotonicTime();
		device.Stream.Stop();
		assert(AudioMonotonicTime() - start < 0.1);
	}

}
int main()
{
	Run(44100, 512, 48000, 200);
	Run(48000, 128, 44100, -200);
	Run(48000, 384, 48000, 0);
	ClockTransitions();
	WorkerShutdown();
	StallRecovery();
}
