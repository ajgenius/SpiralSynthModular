#include "AudioStream.h"
#include "AudioTimeline.h"
#include "PresentationClock.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <unistd.h>
#include <new>
#include <cstdlib>
using namespace Spiral::Core;
#if __cplusplus >= 201103L
#define ALLOCATION_THROWS noexcept(false)
#else
#define ALLOCATION_THROWS throw(std::bad_alloc)
#endif

static bool insideCallback = false;
void *operator new(std::size_t bytes) ALLOCATION_THROWS
{
	assert(!insideCallback);
	void *value = std::malloc(bytes ? bytes : 1);
	if (!value) throw std::bad_alloc();

	return value;
}
void *operator new[](std::size_t bytes) ALLOCATION_THROWS { return ::operator new(bytes); }
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

class TransportDevice : public Device
{
public:
	TransportDevice() : Device(64, 44100, 0, 0, 0), position(44100), rolling(false) {}
	unsigned long position;
	bool rolling;
	bool GetTransport(unsigned long &frame, bool &playing) const { frame = position; playing = rolling; return true; }
	bool StartTransport() { return rolling = true; }
	bool StopTransport() { rolling = false; return true; }
	bool LocateTransport(unsigned long frame) { position = frame; return true; }
};

static void ExternalTransport()
{
	Device clock(64, 48000, 0, 0, 0);
	TransportDevice jack;
	assert(clock.Stream.Configure(&clock, 0, 2, 64, 48000));
	assert(jack.Stream.Configure(&jack, 0, 2, 64, 48000));
	AudioTimeline timeline;
	timeline.Register(&clock.Stream);
	timeline.Register(&jack.Stream);
	clock.Tick();
	jack.Tick();
		bool ready = false;
	for (unsigned n = 0; n < 100 && !ready; ++n)
		ready = timeline.Prepare(64, 48000, 10 + n * 0.001, &clock.Stream);
	assert(ready);
	timeline.Begin();
	assert(timeline.Rolling() && timeline.Frame() == 0);
	timeline.FollowExternalTransport(true);
	timeline.Begin();
	assert(!timeline.Rolling() && timeline.Frame() == 48000);
	timeline.Start();
	assert(jack.rolling);
	timeline.Locate(96000);
	assert(jack.position == 88200);
	timeline.Unregister(&clock.Stream);
	timeline.Begin();
	assert(timeline.Rolling() && timeline.Frame() == 96000);
	timeline.Stop();
	timeline.Begin();
	assert(!timeline.Rolling() && !jack.rolling);
	timeline.Unregister(&jack.Stream);
}

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
	native.Epoch = 2;
	native.Frame += 480;
	native.CallbackTime = native.OutputTime = 106.02;
	if (!clock.Prepare(106.02, 480, 48000, &native, 0.04, stamp))
		assert(clock.Prepare(106.0201, 480, 48000, &native, 0.04, stamp));

	assert(stamp.Generation != generation && stamp.Frame == 1920);

	const unsigned queuedGeneration = stamp.Generation;
	const double queuedEnd = stamp.Time + 480 * stamp.Step;
	clock.Commit(480);
	native.PlaybackRestart = true;
	++native.Epoch;
	native.CallbackTime = native.OutputTime = 106.03;
	assert(clock.Prepare(106.0301, 480, 48000, &native, 0.04, stamp));
	assert(stamp.Generation == queuedGeneration && stamp.Frame == 2400);
	assert(std::fabs(stamp.Time - queuedEnd) < 1e-9);

	clock.Commit(480);
	++native.Epoch;
	native.CallbackTime = native.OutputTime = 107;
	if (!clock.Prepare(107, 480, 48000, &native, 0.04, stamp))
		assert(clock.Prepare(107.0001, 480, 48000, &native, 0.04, stamp));
	assert(stamp.Generation != queuedGeneration && stamp.Frame == 2880);
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

class PreparedDevice : public Device, public PreparedPlayback
{
public:
	PreparedDevice() : Device(64, 48000, 0, 0, 0), Writes(0), Queries(0) {}

	bool PreparedTiming(AudioCycleTiming &timing) const
	{
		__sync_fetch_and_add(&Queries, 1);
		timing.Valid = timing.Estimated = true;
		timing.Frames = Period;
		timing.SampleRate = Rate;
		timing.Step = 1 / Rate;
		timing.CallbackTime = timing.OutputTime = Next;
		return true;
	}

	int WaitForCycle(unsigned)
	{
		const unsigned period = Written();
		if (period == 4)
			return 0;

		PreparedTiming(Cycle);
		Cycle.Frame = period * Period;
		Cycle.OutputTime = Next + Cycle.Frame / Rate;
		return 1;
	}

	double GetChannelTime(bool, unsigned) const { return Cycle.OutputTime; }

	bool Write(const float *samples, unsigned frames)
	{
		assert(frames == Period && Written() < 4);
		std::copy(samples, samples + 2 * frames, Output + 2 * frames * Written());
		__sync_fetch_and_add(&Writes, 1);
		return true;
	}

	unsigned Written() const { return __sync_fetch_and_add(&Writes, 0); }
	unsigned PreparedQueries() const { return __sync_fetch_and_add(&Queries, 0); }
	float Output[4 * 64 * 2];

private:
	mutable unsigned Writes, Queries;
};

class CallbackDevice : public PreparedDevice
{
public:
	bool IsCallbackDriven() const { return true; }
};

static void ColdCallbackPlayback()
{
	CallbackDevice device;
	assert(device.Stream.Configure(&device, 0, 2, 64, 48000));
	assert(device.Stream.Start());
	device.Tick();
	assert(device.Stream.Errors() == 0);

	AudioStamp stamp;
	stamp.Generation = 1;
	stamp.Step = 1.0 / 48000;
	stamp.Time = device.Next + 64 * stamp.Step;
	float samples[128];
	std::fill_n(samples, 128, 0.75f);
	device.Stream.SetGeneration(stamp.Generation, stamp.Time);
	assert(device.Stream.Playback(samples, 64, stamp));
	device.Tick();

	// A latency change supersedes the queued block before it reaches the DAC.
	++stamp.Generation;
	stamp.Time = device.Next + 17 * stamp.Step;
	device.Stream.SetGeneration(stamp.Generation, stamp.Time);
	for (unsigned n = 0; n < 64; ++n)
	{
		samples[2*n] = 0.25f;
		samples[2*n+1] = -0.5f;
	}
	assert(device.Stream.Playback(samples, 64, stamp));
	device.Tick();
	assert(device.Stream.Errors() == 0);
	device.Stream.SetGeneration(stamp.Generation, device.Next + 1);
	device.Tick();

	// Withhold the next block. Its deadline still counts even when part of
	// that period was supplied by the first block.
	assert(device.Written() == 4 && device.Stream.Errors() == 1);
	for (unsigned frame = 0; frame < 256; ++frame)
	{
		const bool scheduled = frame >= 145 && frame < 209;
		assert(std::fabs(device.Output[2*frame] - (scheduled ? 0.25f : 0)) < 1e-5);
		assert(std::fabs(device.Output[2*frame+1] - (scheduled ? -0.5f : 0)) < 1e-5);
	}

	CallbackDevice late;
	assert(late.Stream.Configure(&late, 0, 2, 64, 48000));
	assert(late.Stream.Start());
	late.Stream.SetGeneration(1, late.Next);
	late.Tick();
	assert(late.Written() == 1 && late.Stream.Errors() == 1);
}

static void PatchReplacement()
{
	PreparedDevice device;
	assert(device.Stream.Configure(&device, 0, 2, 64, 48000));
	device.Stream.SetGeneration(1);
	AudioStamp stamp;
	stamp.Generation = 1;
	stamp.Step = 1.0 / 48000;
	stamp.Time = device.Next;
	float samples[128];
	std::fill(samples, samples + 128, 0.25f);
	assert(device.Stream.Playback(samples, 64, stamp));

	device.Stream.BeginPatchReplacement();
	device.Tick();
	device.Stream.EndPatchReplacement();
	device.Stream.SetGeneration(2);
	device.Tick();
	stamp.Generation = 2;
	stamp.Frame = 128;
	stamp.Time = device.Next + 81 * stamp.Step;
	std::fill(samples, samples + 128, 0.75f);
	assert(device.Stream.Playback(samples, 64, stamp));
	device.Tick();
	device.Tick();

	assert(device.Written() == 4 && device.Stream.Errors() == 0);
	for (unsigned frame = 0; frame < 256; ++frame)
	{
		const float expected = frame < 209 ? 0.0f : 0.75f;
		assert(std::fabs(device.Output[2 * frame] - expected) < 1e-5);
		assert(std::fabs(device.Output[2 * frame + 1] - expected) < 1e-5);
	}
}

static void ColdPlayback(bool replaceQueuedAudio)
{
	PreparedDevice device;
	device.Next = 3384237.1991879847;
	assert(device.Stream.Configure(&device, 0, 2, 64, 48000));

	// Stop must also work while the PCM is waiting for its first graph block.
	assert(device.Stream.Start());
	AudioCycleTiming timing;
	assert(device.Stream.Timing(timing) && timing.Valid);
	usleep(5000);
	device.Stream.Stop();
	assert(device.Written() == 0);

	AudioStamp stamp;
	stamp.Generation = 2;
	stamp.Step = 1.0 / 48000;
	stamp.Time = (device.Next + 64 * stamp.Step) + 16 * stamp.Step;
	float samples[128];
	if (replaceQueuedAudio)
	{
		AudioStamp old = stamp;
		--old.Generation;
		device.Stream.SetGeneration(old.Generation);
		std::fill_n(samples, 128, 0.75f);
		while (device.Stream.Playback(samples, 64, old))
		{
			old.Frame += 64;
			old.Time += 64 * old.Step;
		}
	}
	device.Stream.SetGeneration(stamp.Generation);
	const unsigned queries = device.PreparedQueries();
	assert(device.Stream.Start());
	if (replaceQueuedAudio)
	{
		// Wait for a prepared-clock refresh after the worker's first scan.
		for (unsigned n = 0; n < 1000 && device.PreparedQueries() < queries + 3; ++n)
			usleep(1000);
		assert(device.PreparedQueries() >= queries + 3);
	}
	for (unsigned frame = 0; frame < 64; ++frame)
	{
		samples[2 * frame] = 0.25f;
		samples[2 * frame + 1] = -0.5f;
	}
	const unsigned producerErrors = device.Stream.Errors();
	assert(device.Stream.Playback(samples, 64, stamp));

	for (unsigned n = 0; n < 1000 && device.Written() < 4; ++n)
		usleep(1000);

	device.Stream.Stop();
	assert(device.Written() == 4);
	for (unsigned frame = 0; frame < 256; ++frame)
	{
		const bool scheduled = frame >= 80 && frame < 144;
		assert(std::fabs(device.Output[2 * frame] - (scheduled ? 0.25f : 0)) < 0.0001);
		assert(std::fabs(device.Output[2 * frame + 1] - (scheduled ? -0.5f : 0)) < 0.0001);
	}

	// The graph deliberately starves the third and fourth native periods.
	assert(device.Stream.Errors() == producerErrors + 2 && !device.Stream.Failed());
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
	ExternalTransport();
	Run(44100, 512, 48000, 200);
	Run(48000, 128, 44100, -200);
	Run(48000, 384, 48000, 0);
	ClockTransitions();
	ColdCallbackPlayback();
	ColdPlayback(false);
	ColdPlayback(true);
	WorkerShutdown();
	PatchReplacement();
	StallRecovery();
}
