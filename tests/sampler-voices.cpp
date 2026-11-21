// SPDX-License-Identifier: GPL-2.0-or-later
#include "PoshSamplerPlugin.h"
#include "DeviceClassRegistry.h"
#include "RiffWav.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <unistd.h>

using namespace Spiral::Core;

static PoshSamplerPlugin *second;

static SpiralPlugin *MakeCopy()
{
	second = new PoshSamplerPlugin;
	return second;
}

static SpiralPlugin *Lookup(int, void *sampler)
{
	return static_cast<PoshSamplerPlugin *>(sampler);
}

static float Cursor(PoshSamplerPlugin &sampler)
{
	return sampler.SampleFloat(0, PoshSamplerPlugin::SLOT_SAMPLEPOS);
}

static void CheckOutput(PoshSamplerPlugin &sampler, int start, float gain)
{
	Sample *out = 0;
	assert(sampler.GetOutput(1, &out));
	for (int n = 0; n < 64; ++n)
		assert((*out)[n] == (*sampler.GetSample(0))[start + n] * gain);
}

static void CheckReload(const Description &saved, const HostInfo &host, const std::string &dir)
{
	PoshSamplerPlugin restored;
	restored.Initialise(&host);
	Description::Reader fields(saved);
	restored.Apply(fields);
	assert(!fields.Failed());
	if (!saved.Named())
		assert(!fields.More());

	restored.LoadExternalFiles(dir, 17);
	restored.Reset();
	assert(Cursor(restored) == 117 && restored.GetVolume(0) == .5f);
	assert(!restored.GetReTrig(0));
	restored.Execute();
	CheckOutput(restored, 117, .5f);
}

static void CheckNoteTrigger(const HostInfo &host, const std::string &dir)
{
	PoshSamplerPlugin sampler;
	sampler.Initialise(&host);
	sampler.LoadExternalFiles(dir, 17);
	sampler.Reset();

	Sample pitch(64), gate(64);
	pitch.Zero();
	gate.Zero();
	sampler.SampleSigned(0, PoshSamplerPlugin::SLOT_NOTE) = 131;
	sampler.Execute();
	assert(Cursor(sampler) == -1);
	sampler.SampleSigned(0, PoshSamplerPlugin::SLOT_NOTE) = 69;
	pitch.Set(4, 2 * 440.f / MAX_FREQ - 1);
	assert(sampler.SetInput(17, &pitch));
	sampler.Execute();
	Sample *mixed = 0, *slot = 0;
	assert(sampler.GetOutput(0, &mixed) && sampler.GetOutput(1, &slot));
	assert((*slot)[5] == (*sampler.GetSample(0))[1] && (*mixed)[5] == (*slot)[5]);

	gate.Set(4, .25f);
	assert(sampler.SetInput(1, &gate));
	sampler.Execute();
	assert((*slot)[5] > 0 && (*mixed)[5] == .25f * (*slot)[5]);
}

static void CheckPCM16(PoshSamplerPlugin &sampler, const std::string &file)
{
	Sample &source = *sampler.GetSample(0);
	source.Allocate(65536);
	for (int i = 0; i < 65536; ++i)
		source.Set(i, (i - 32768) / 32768.f);

	sampler.m_SampleDescVec[0]->SampleRate = 32000;
	sampler.SaveSample(0, file);

	// An independent RIFF reader must be able to reach every audio frame.
	std::ifstream bytes(file.c_str(), std::ios::binary | std::ios::ate);
	const std::streamoff size = bytes.tellg();
	bytes.seekg(4);
	unsigned char length[4];
	assert(bytes.read(reinterpret_cast<char *>(length), sizeof(length)));
	const unsigned riffBytes = length[0] | unsigned(length[1]) << 8
		| unsigned(length[2]) << 16 | unsigned(length[3]) << 24;
	assert(size == 8 + std::streamoff(riffBytes));

	sampler.LoadSample(1, file);
	assert(sampler.m_SampleDescVec[1]->SampleRate == 32000);
	const Sample &saved = *sampler.GetSample(1);
	assert(saved.GetLength() == 65536);
	for (int i = 0; i < 65536; ++i)
		assert(saved[i] == source[i]);
}

// The sampler's voices, as ajgenius set them on 10 November 2026: a trigger
// for each sample, as it always had; one playhead for each sample; and the
// voices on the mixed output only, where each sample that is playing has one
// while there are enough. The sampler runs once and is not copied.
int main()
{
	char temp[] = "/tmp/ssm-sampler-voices-XXXXXX";
	assert(mkdtemp(temp));
	const std::string dir = std::string(temp) + "/";
	const std::string sidecar = dir + "PoshSampler17_0.wav";
	Sample ramp(512);
	for (int n = 0; n < 512; ++n)
		ramp.Set(n, n / 1024.0f);

	WavFile wav;
	wav.SetSamplerate(48000);
	assert(wav.Open(sidecar, WavFile::WRITE));
	wav.Save(ramp);
	wav.Close();

	HostInfo host = HostInfo();
	host.BUFSIZE = 64;
	host.SAMPLERATE = 48000;
	CheckNoteTrigger(host, dir);
	PoshSamplerPlugin first;
	first.Initialise(&host);
	first.LoadExternalFiles(dir, 17);
	DeviceClass kind;
	kind.Create = MakeCopy;
	VoicePool pool;
	pool.SetLookup(Lookup, &first);
	pool.Add(17, &first, kind, 2, &host, dir);
	first.Reset();

	// The one device has both voices of its mixed output.
	assert(!second && pool.Holds(17));
	assert(first.VoiceOutput(0, 0) && first.VoiceOutput(0, 1) && !first.VoiceOutput(0, 2));

	// A gate on either voice of what is wired to a sample's trigger starts
	// it. The first voice's input is the port's own; the pool hands over
	// the second's.
	Sample a(64), b(64);
	a.Zero();
	b.Zero();
	assert(first.SetInput(1, &a));
	first.SetVoiceInput(1, 1, &b);
	b.Set(3, 1);
	first.Execute();
	assert(Cursor(first) == 61);

	// A sample has one playhead: a gate on the other voice while it plays
	// starts the same sample again, and it does not play beside itself.
	b.Zero();
	a.Set(11, 1);
	first.Execute();
	assert(Cursor(first) == 53);
	assert(pool.Follow());
	FloatProperty volume(DefaultLinearFlags, .5f, 0, 2, .001f, .1f);
	first.SynchronizeProperty("VOLUME0", &volume);
	assert(pool.Follow());
	assert(first.GetVolume(0) == .5f && Cursor(first) == 53);

	a.Zero();
	first.Execute();
	CheckOutput(first, 53, .5f);

	// It is on the first voice of the mixed output, and the second is silent.
	for (int n = 0; n < 64; ++n)
		assert((*first.VoiceOutput(0, 0))[n] == ramp[53 + n] * .5f && (*first.VoiceOutput(0, 1))[n] == 0);

	// A saved version 4 patch still stores the cursor in its original slot.
	ToggleProperty retrigger(Property::WriteOnly, false);
	first.SynchronizeProperty("RETRIG0", &retrigger);
	Description saved;
	first.Describe(saved);
	assert(saved.Version() == 4 && saved.Values().size() == 1 + 12 * NUM_SAMPLES);
	assert(saved.Values()[7] == "117");
	Description fromFile;
	std::string error;
	assert(Description::FromJSON(saved.JSON(), fromFile, error));
	CheckReload(fromFile, host, dir);

	Description legacy;
	for (size_t i = 0; i < saved.Values().size(); ++i)
		legacy.Value(saved.Values()[i]);

	CheckReload(legacy, host, dir);

	// Wired afresh, the sampler keeps where it is: there is no new copy to start.
	pool.Clear();
	pool.Add(17, &first, kind, 2, &host, dir);
	assert(!second && Cursor(first) == 117);
	first.SetVoiceInput(1, 1, &b);
	retrigger.Value.AsBoolean = true;
	first.SynchronizeProperty("RETRIG0", &retrigger);

	// A start CV sets where this attack begins and leaves the panel's marker.
	Sample startCV(64);
	startCV.Zero(); // Middle of the sample.
	assert(first.SetInput(18, &startCV));
	a.Set(0, 1);
	first.Execute();
	assert(Cursor(first) == 255 + 64 && first.GetPlayStart(0) == 0);
	a.Zero();

	// Each sample that is playing has a voice of the mixed output. A second
	// sample, loaded at half the rate and edited in place, takes the other
	// voice while the first goes on on its own.
	wav.SetSamplerate(24000);
	assert(wav.Open(sidecar, WavFile::WRITE));
	wav.Save(ramp);
	wav.Close();
	first.LoadSample(1, sidecar);
	first.GetSample(1)->Set(10, -.5f);
	Sample gate(64);
	gate.Zero();
	gate.Set(0, 1);
	assert(first.SetInput(3, &gate));

	// Where a wire leaves the pool the voices are added together.
	const VoicePool::Wire out = {17, 0, 99, 16};
	pool.SumOut(out, 64);
	first.Execute();
	pool.Run(17, &first, false, false, 256);
	Sample *one = 0, *two = 0;
	assert(first.GetOutput(1, &one) && first.GetOutput(2, &two));
	for (int n = 0; n < 64; ++n)
	{
		if (n % 2 == 0)
			assert((*two)[n] == (n == 20 ? -.5f : ramp[n / 2]));

		assert((*first.VoiceOutput(0, 0))[n] == (*one)[n] && (*one)[n] > 0);
		assert((*first.VoiceOutput(0, 1))[n] == (*two)[n]);
		assert((*first.GetInput(16))[n] == (*one)[n] + (*two)[n]);
		// A sample's own output is one voice: the other has silence.
		assert((*first.VoiceOutput(2, 1))[n] == 0);
	}
	assert(first.SampleFloat(1, PoshSamplerPlugin::SLOT_SAMPLEPOS) == 32);

	CheckPCM16(first, sidecar);
	assert(unlink(sidecar.c_str()) == 0);
	assert(rmdir(temp) == 0);
}
