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

class AudioTap : public SpiralPlugin
{
public:
	AudioTap() { m_PluginInfo.NumInputs = 2; m_PluginInfo.NumOutputs = 1; }
	void Execute() { *GetOutputBuf(0) = *GetInput(0); }
	void Describe(Description &) {}
	void Apply(Description::Reader &) {}
};

static SpiralPlugin *MakeTap() { return new AudioTap; }

struct SamplerPatch
{
	PoshSamplerPlugin Sampler;
	AudioTap Effect, Sink;
};

static SpiralPlugin *PatchLookup(int id, void *patch)
{
	SamplerPatch &p = *static_cast<SamplerPatch *>(patch);
	if (id == 17)
		return &p.Sampler;
	return id == 18 ? &p.Sink : &p.Effect;
}

static Sample *LaneOutput(PoshSamplerPlugin &sampler, unsigned port, unsigned lane)
{
	return sampler.VoiceOutput(port, lane);
}

static void CheckFrame(PoshSamplerPlugin &sampler, unsigned frame, unsigned lane,
	int slot, unsigned age, float velocity)
{
	const float expected = slot < 0 ? 0.f : (*sampler.GetSample(slot))[age];
	assert((*LaneOutput(sampler, 0, lane))[frame] == expected * velocity);
	if (slot >= 0)
		assert((*LaneOutput(sampler, slot + 1, 0))[frame] == expected);
}

static void CheckAllocation(const HostInfo &host)
{
	SamplerPatch patch;
	PoshSamplerPlugin &sampler = patch.Sampler;
	sampler.Initialise(&host);
	patch.Sink.Initialise(&host);
	patch.Effect.Initialise(&host);
	DeviceClass kind;
	kind.Create = MakeCopy;
	VoicePool pool;
	pool.SetLookup(PatchLookup, &patch);
	pool.Add(17, &sampler, kind, 3, &host, "");
	assert(!second && sampler.VoiceOutput(0, 2) && !sampler.VoiceOutput(0, 3));
	const VoicePool::Wire wire = {17, 0, 18, 0};
	pool.SumOut(wire, 64);

	// Also send the sampler through an ordinary effect in the same pool.
	// Its summed audio must match the sampler-to-mono wire sample for sample.
	kind.Create = MakeTap;
	pool.Add(19, &patch.Effect, kind, 3, &host, "");
	assert(patch.Effect.SetInput(0, sampler.VoiceOutput(0, 0)));
	const VoicePool::Wire intoEffect = {17, 0, 19, 0};
	pool.Feed(intoEffect, true);
	const VoicePool::Wire effectsMix = {19, 0, 18, 1};
	pool.SumOut(effectsMix, 64);
	Sample inputs[4];
	Sample *gates[4];
	for (unsigned s = 0; s < 4; ++s)
	{
		gates[s] = &inputs[s];
		gates[s]->Allocate(64);
		gates[s]->Zero();
		assert(sampler.SetInput(2 * s + 1, gates[s]));
	}
	for (unsigned s = 0; s < 4; ++s)
	{
		Sample &pcm = *sampler.GetSample(s);
		pcm.Allocate(256);
		for (unsigned n = 0; n < 256; ++n)
			pcm.Set(n, (s + 1) * .125f + n / 2048.f);
		sampler.m_SampleDescVec[s]->SampleRate = 48000;
	}
	// The second gate stays high across the steal and the block boundary.
	gates[0]->Set(2, .5f);
	gates[1]->Set(4, .5f);
	gates[2]->Set(6, .5f);
	gates[0]->Set(7, .75f);
	gates[3]->Set(8, .5f);
	for (unsigned n = 5; n < 64; ++n)
		gates[1]->Set(n, .5f);
	sampler.Execute();
	pool.Run(17, &sampler, true, false, 0);
	patch.Effect.Execute();
	pool.Run(19, &patch.Effect, true, false, 0);
	for (unsigned n = 0; n < 64; ++n)
	{
		CheckFrame(sampler, n, 0, n < 2 ? -1 : 0, n < 7 ? n - 2 : n - 7, n < 7 ? .5f : .75f);
		CheckFrame(sampler, n, 1, n < 4 ? -1 : n < 8 ? 1 : 3, n < 8 ? n - 4 : n - 8, .5f);
		CheckFrame(sampler, n, 2, n < 6 ? -1 : 2, n - 6, .5f);
		float sum = 0;
		for (unsigned v = 0; v < 3; ++v)
			sum += (*LaneOutput(sampler, 0, v))[n];
		assert((*patch.Sink.GetInput(0))[n] == sum);
		assert((*patch.Sink.GetInput(1))[n] == sum);
	}
	for (unsigned s = 0; s < 4; ++s)
		gates[s]->Zero();
	gates[1]->Set(0, .5f);
	gates[1]->Set(2, 1.f);
	sampler.Execute();
	pool.Run(17, &sampler, true, false, 0);
	for (unsigned n = 0; n < 64; ++n)
	{
		CheckFrame(sampler, n, 0, 0, n + 57, .75f);
		CheckFrame(sampler, n, 1, 3, n + 56, .5f);
		CheckFrame(sampler, n, 2, n < 2 ? 2 : 1, n < 2 ? n + 58 : n - 2, n < 2 ? .5f : 1.f);
	}
	assert((*LaneOutput(sampler, 3, 0))[2u] == 0);

	// Slot four ends exactly as slot three attacks. Reuse the freed lane,
	// even though the new slot's trigger is visited first.
	for (unsigned s = 0; s < 4; ++s)
		gates[s]->Zero();
	gates[2]->Set(0, 1.f);
	sampler.SampleSigned(3, PoshSamplerPlugin::SLOT_LOOPEND) = 120;
	sampler.Execute();
	CheckFrame(sampler, 0, 0, 0, 121, .75f);
	CheckFrame(sampler, 0, 1, 2, 0, 1.f);
	CheckFrame(sampler, 0, 2, 1, 62, 1.f);

	pool.Clear();
	assert(!sampler.VoiceOutput(0, 1) && patch.Sink.GetInput(0) == sampler.VoiceOutput(0, 0));
	assert(sampler.SampleFloat(2, PoshSamplerPlugin::SLOT_SAMPLEPOS) == -1);
	sampler.SetVoice(&sampler, 0, 12);
	assert(sampler.VoiceOutput(0, 7) && !second);
	assert((*sampler.VoiceOutput(0, 9))[0] == 0);
}


static void CheckOneFrame(const HostInfo &host, const std::string &file)
{
	Sample hit(1), trigger(64);
	hit.Set(0, .5f);
	WavFile wav;
	wav.SetSamplerate(host.SAMPLERATE);
	assert(wav.Open(file, WavFile::WRITE));
	wav.Save(hit);
	wav.Close();

	PoshSamplerPlugin sampler;
	sampler.Initialise(&host);
	sampler.LoadSample(0, file);
	sampler.Reset();
	trigger.Zero();
	trigger.Set(63, 1.f);
	assert(sampler.SetInput(1, &trigger));
	sampler.Execute();
	const float value = (*sampler.GetSample(0))[0];
	assert(value > .49f && (*sampler.VoiceOutput(0, 0))[63] == value);
	trigger.Zero();
	sampler.Execute();
	assert((*sampler.VoiceOutput(0, 0))[0] == 0);
	sampler.SampleFloat(0, PoshSamplerPlugin::SLOT_PITCHMOD) = .5f;
	trigger.Set(1, 1.f);
	sampler.Execute();
	const Sample &mix = *sampler.VoiceOutput(0, 0);
	assert(mix[1] == value && mix[2] == value && mix[3] == 0);
}

// In a patch of more voices than the sampler has samples the mixed output has
// a voice for each sample and no more. All eight started together take one
// each, and the voices past them are there and silent.
static void CheckWidePatch(const HostInfo &host, const std::string &dir)
{
	PoshSamplerPlugin sampler;
	sampler.Initialise(&host);
	DeviceClass kind;
	kind.Create = MakeCopy;
	VoicePool pool;
	pool.SetLookup(Lookup, &sampler);
	pool.Add(17, &sampler, kind, 12, &host, dir);
	sampler.Reset();

	Sample gate(64);
	gate.Zero();
	gate.Set(0, 1);
	for (int s = 0; s < NUM_SAMPLES; ++s)
	{
		Sample &pcm = *sampler.GetSample(s);
		pcm.Allocate(256);
		for (int n = 0; n < 256; ++n)
			pcm.Set(n, (s + 1) / 16.f);

		sampler.m_SampleDescVec[s]->SampleRate = 48000;
		sampler.SampleSigned(s, PoshSamplerPlugin::SLOT_LOOPEND) = 255;
		assert(sampler.SetInput(2 * s + 1, &gate));
	}

	sampler.Execute();
	for (int s = 0; s < NUM_SAMPLES; ++s)
		assert((*sampler.VoiceOutput(0, s))[5] == (s + 1) / 16.f);

	for (unsigned v = NUM_SAMPLES; v < 12; ++v)
		assert(sampler.VoiceOutput(0, v) && (*sampler.VoiceOutput(0, v))[5] == 0);

	assert(!sampler.VoiceOutput(0, 12));
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
	CheckWidePatch(host, dir);
	CheckAllocation(host);
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

	Sample a(64);
	a.Zero();
	assert(first.SetInput(1, &a));
	a.Set(3, 1);
	first.Execute();
	assert(Cursor(first) == 61);

	a.Zero();
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

	// The saved patch keeps the cursor in its original slot.
	ToggleProperty retrigger(Property::WriteOnly, false);
	first.SynchronizeProperty("RETRIG0", &retrigger);
	Description saved;
	first.Describe(saved);
	assert(saved.Version() == 5 && saved.Values().size() == 1 + 12 * NUM_SAMPLES);
	assert(saved.Values()[7] == "117");
	Description fromFile;
	std::string error;
	assert(Description::FromJSON(saved.JSON(), fromFile, error));
	CheckReload(fromFile, host, dir);

	Description legacy;
	for (size_t i = 0; i < saved.Values().size(); ++i)
		legacy.Value(saved.Values()[i]);

	CheckReload(legacy, host, dir);

	// Version 4 used frame 511 as the whole 512-frame sample. Version 5
	// can save that exact value as an intentional trim.
	for (int version = 4; version <= 5; ++version)
	{
		Description trimmed;
		trimmed.Value(version);
		for (size_t i = 1; i < saved.Values().size(); ++i)
			trimmed.Value(i == 10 ? "511" : saved.Values()[i]);
		PoshSamplerPlugin loaded;
		loaded.Initialise(&host);
		Description::Reader reader(trimmed);
		loaded.Apply(reader);
		loaded.LoadExternalFiles(dir, 17);
		loaded.Reset();
		assert(loaded.GetLoopEnd(0) == (version == 4 ? 512 : 511));
	}

	// Wired afresh, the sampler keeps where it is: there is no new copy to start.
	pool.Clear();
	pool.Add(17, &first, kind, 2, &host, dir);
	assert(!second && Cursor(first) == 117);
	retrigger.Value.AsBoolean = true;
	first.SynchronizeProperty("RETRIG0", &retrigger);

	// A start CV sets where this attack begins and leaves the panel's marker.
	Sample startCV(64);
	startCV.Zero(); // Middle of the sample.
	assert(first.SetInput(18, &startCV));
	a.Set(0, 1);
	first.Execute();
	assert(Cursor(first) == 256 + 64 && first.GetPlayStart(0) == 0);
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

	CheckOneFrame(host, sidecar);
	CheckPCM16(first, sidecar);
	assert(unlink(sidecar.c_str()) == 0);
	assert(rmdir(temp) == 0);
}
