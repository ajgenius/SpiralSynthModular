// SPDX-License-Identifier: GPL-2.0-or-later
#include "TimedAudioBuffer.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <pthread.h>
#include <vector>

using namespace spiralcore;
static const double Pi = 3.14159265358979323846;

static void Rates(double sourceRate, double deviceRate, unsigned sourcePeriod, unsigned devicePeriod)
{
	TimedAudioBuffer queue;
	assert(queue.Configure(32768, 2, sourceRate, deviceRate));
	std::vector<float> source(sourcePeriod * 2), output(devicePeriod * 2);
	AudioStamp stamp;
	stamp.Generation = 1;
	stamp.Step = 1 / sourceRate;
	const double origin = 12345;
	unsigned produced = 0;
	unsigned missing = 0;
	double error = 0;

	// Ten seconds of independently clocked devices, with arbitrary callback
	// phase and unequal periods. The signal's phase is defined by time, not
	// by queue occupancy; no real-time sleeps or audio hardware are involved.
	for (unsigned cycle = 0; cycle < unsigned(deviceRate * 10 / devicePeriod); ++cycle)
	{
		const double time = origin + (cycle * double(devicePeriod) + 0.371) / deviceRate;
		while (produced / sourceRate < time - origin + (devicePeriod + 256) / deviceRate)
		{
			stamp.Frame = produced;
			stamp.Time = origin + produced / sourceRate;
			for (unsigned n = 0; n < sourcePeriod; ++n)
			{
				source[n * 2] = float(std::sin(2 * Pi * 997 * (produced + n) / sourceRate));
				source[n * 2 + 1] = -source[n * 2];
			}

			assert(queue.Write(&source[0], sourcePeriod, stamp));
			produced += sourcePeriod;
		}

		const unsigned delivered = queue.Read(&output[0], devicePeriod, time, 1 / deviceRate, 1);
		if (cycle > 2)
		{
			missing += devicePeriod - delivered;
			for (unsigned n = 0; n < devicePeriod; ++n)
			{
				const double expected = std::sin(2 * Pi * 997 * (time - origin + n / deviceRate));
				error = std::max(error, std::fabs(output[n * 2] - expected));
				assert(output[n * 2] == -output[n * 2 + 1]);
			}
		}

		assert(queue.Buffered() < 4096);
	}

	printf("%.3f -> %.3f Hz, %u/%u periods: missing %u, peak error %.6f\n",
		sourceRate, deviceRate, sourcePeriod, devicePeriod, missing, error);
	assert(!missing && error < 0.0002);
}

static void HolesAndGenerations()
{
	TimedAudioBuffer queue;
	assert(queue.Configure(512, 1, 48000, 48000));
	float input[128], output[128];
	for (unsigned n = 0; n < 128; ++n) input[n] = float(n + 1);

	AudioStamp stamp;
	stamp.Generation = 1;
	stamp.Time = 10;
	stamp.Step = 1.0 / 48000;
	assert(queue.Write(input, 128, stamp));
	stamp.Frame = 256;
	stamp.Time += 256 * stamp.Step;
	assert(queue.Write(input, 128, stamp));
	assert(queue.Read(output, 128, 10, stamp.Step, 1) == 128);
	assert(output[0] == 1 && output[127] == 128);
	assert(queue.Read(output, 128, 10 + 128 * stamp.Step, stamp.Step, 1) == 0);
	assert(queue.Read(output, 128, stamp.Time, stamp.Step, 1) == 128);
	assert(output[0] == 1 && output[127] == 128);

	// Seek/reconnect invalidates old queued audio, without either side moving
	// the other's cursor. A reader that still holds the old generation must
	// not throw away already published future audio.
	stamp.Generation = 2;
	stamp.Frame = 384;
	stamp.Time += 128 * stamp.Step;
	assert(queue.Write(input, 128, stamp));
	queue.Read(output, 128, stamp.Time, stamp.Step, 1);
	assert(queue.Read(output, 128, stamp.Time, stamp.Step, 2) == 128);
	assert(output[0] == 1 && output[127] == 128);
}

static void AntiAlias()
{
	TimedAudioBuffer queue;
	assert(queue.Configure(8192, 1, 48000, 8000));
	std::vector<float> input(6144), output(512);
	for (unsigned n = 0; n < input.size(); ++n)
		input[n] = float(std::sin(2 * Pi * 6000 * n / 48000));

	AudioStamp stamp;
	stamp.Time = 10;
	stamp.Step = 1.0 / 48000;
	stamp.Generation = 1;
	assert(queue.Write(&input[0], input.size(), stamp));
	assert(queue.Read(&output[0], output.size(), 10 + 1024.0 / 48000, 1.0 / 8000, 1) == output.size());
	double peak = 0;
	for (unsigned n = 0; n < output.size(); ++n)
		peak = std::max(peak, std::fabs(double(output[n])));

	printf("48 -> 8 kHz anti-alias: 6 kHz rejection peak %.8f\n", peak);
	assert(peak < 0.0001);
}

struct MailboxStress
{
	AudioTimingMailbox Mailbox;
	unsigned Done;
	MailboxStress() : Done(0) {}
	static void *Publish(void *context)
	{
		MailboxStress &test = *static_cast<MailboxStress *>(context);
		for (unsigned n = 1; n <= 100000; ++n)
		{
			AudioCycleTiming value;
			value.Frame = n;
			value.Frames = n;
			value.OutputTime = n * 0.5;
			value.Valid = true;
			test.Mailbox.Publish(value);
		}

		__sync_lock_test_and_set(&test.Done, 1);
		return NULL;
	}
};

int main()
{
	Rates(44100, 48000, 512, 128);
	Rates(48000, 44100, 128, 512);
	Rates(48000, 48000 * 1.0002, 512, 384);
	Rates(48000, 48000 * 0.9998, 384, 512);
	HolesAndGenerations();
	AntiAlias();

	MailboxStress test;
	pthread_t writer;
	assert(!pthread_create(&writer, NULL, MailboxStress::Publish, &test));
	AudioCycleTiming value;
	uint64_t previous = 0;
	do
	{
		if (test.Mailbox.Read(value))
		{
			assert(value.Valid && value.Frame == value.Frames && value.OutputTime == value.Frame * 0.5);
			assert(value.Frame >= previous);
			previous = value.Frame;
		}
	} while (!__sync_fetch_and_add(&test.Done, 0));

	pthread_join(writer, NULL);
	puts("Timed audio rate conversion, gaps, generations and concurrent timing snapshots PASS");
	return 0;
}
