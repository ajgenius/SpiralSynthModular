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
	second->Reset();
	for (int slot = 0; slot < NUM_SAMPLES; ++slot)
		assert(first.GetSample(slot) == second->GetSample(slot));

	Sample a(64), b(64);
	a.Zero();
	b.Zero();
	assert(first.SetInput(1, &a) && second->SetInput(1, &b));
	b.Set(3, 1);
	first.Execute();
	pool.Run(17, &first, true, false, 0);
	b.Zero();
	a.Set(11, 1);
	first.Execute();
	pool.Run(17, &first, true, false, 64);

	assert(Cursor(first) == 53 && Cursor(*second) == 125);
	assert(pool.Follow());
	assert(Cursor(first) == 53 && Cursor(*second) == 125);
	FloatProperty volume(DefaultLinearFlags, .5f, 0, 2, .001f, .1f);
	first.SynchronizeProperty("VOLUME0", &volume);
	assert(pool.Follow());
	assert(first.GetVolume(0) == .5f && second->GetVolume(0) == .5f);
	assert(Cursor(first) == 53 && Cursor(*second) == 125);

	a.Zero();
	first.Execute();
	pool.Run(17, &first, true, false, 128);
	CheckOutput(first, 53, .5f);
	CheckOutput(*second, 125, .5f);

	// A saved version 4 patch still stores the visible cursor in its original slot.
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

	// Expanding a sounding device should not start the new lane.
	pool.Clear();
	pool.Add(17, &first, kind, 2, &host, dir);
	assert(Cursor(first) == 117 && Cursor(*second) == -1);
	second->Reset();
	assert(second->SetInput(1, &b));
	retrigger.Value.AsBoolean = true;
	first.SynchronizeProperty("RETRIG0", &retrigger);
	Sample startCV(64);
	startCV.Zero(); // Middle of the sample.
	assert(first.SetInput(18, &startCV));
	a.Set(0, 1);
	first.Execute();
	assert(Cursor(first) == 255 + 64);
	assert(pool.Follow());
	assert(first.GetPlayStart(0) == 0 && second->GetPlayStart(0) == 0);
	b.Set(0, 1);
	pool.Run(17, &first, true, false, 192);
	CheckOutput(*second, 0, .5f);

	// Replace an empty slot while both lanes exist. Both must read the new
	// waveform at its native rate, including subsequent in-place edits.
	wav.SetSamplerate(24000);
	assert(wav.Open(sidecar, WavFile::WRITE));
	wav.Save(ramp);
	wav.Close();
	first.LoadSample(1, sidecar);
	first.GetSample(1)->Set(10, -.5f);
	Sample gate(64);
	gate.Zero();
	gate.Set(0, 1);
	assert(first.SetInput(3, &gate) && second->SetInput(3, &gate));
	first.Execute();
	pool.Run(17, &first, true, false, 256);
	Sample *left = 0, *right = 0;
	assert(first.GetOutput(2, &left) && second->GetOutput(2, &right));
	for (int n = 0; n < 64; n += 2)
	{
		float expected = n == 20 ? -.5f : ramp[n / 2];
		assert((*left)[n] == expected && (*right)[n] == expected);
	}
	assert(first.SampleFloat(1, PoshSamplerPlugin::SLOT_SAMPLEPOS) == 32);
	assert(second->SampleFloat(1, PoshSamplerPlugin::SLOT_SAMPLEPOS) == 32);

	CheckPCM16(first, sidecar);
	assert(unlink(sidecar.c_str()) == 0);
	assert(rmdir(temp) == 0);
}
