#ifdef TEST_STEREO
#include "StereoMixerPlugin.h"
typedef StereoMixerPlugin Mixer;
static const char GainCommand = Mixer::SETCH;
static const float Scale = 0.5f;
#else
#include "MixerPlugin.h"
typedef MixerPlugin Mixer;
static const char GainCommand = Mixer::SETMIX;
static const float Scale = 1.0f;
#endif
#include <cassert>
#include <limits>
#include <sstream>

static void Control(Mixer &mixer, int channel, float value, char command = GainCommand)
{
	ChannelHandler *ch = mixer.GetChannelHandler();
	ch->Set("Num", channel);
	ch->Set("Value", value);
	ch->SetCommand(command);
	mixer.UpdateChannelHandler();
	mixer.ExecuteCommands();
}

static void Expect(Mixer &mixer, float value)
{
	mixer.Execute();
	for (int port = 0; port < mixer.GetPluginInfo().NumOutputs; ++port)
	{
		Sample *out = NULL;
		assert(mixer.GetOutput(port, &out));
		for (int n = 0; n < 8; ++n) assert((*out)[n] == value * Scale);
	}
}

int main()
{
	HostInfo host = HostInfo();
	host.BUFSIZE = 8;
	Mixer mixer;
	mixer.Initialise(&host);
	Sample good(8), bad(8);
	good.Set(0.25f);
	bad.Set(0.5f);
	assert(mixer.SetInput(0, &good));
	assert(mixer.SetInput(1, &bad));
	Expect(mixer, 0.75f);
	const float invalid[] = {std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};
	for (int i = 0; i < 3; ++i)
	{
		bad.Set(invalid[i]);
		Expect(mixer, 0.25f);
		Control(mixer, 1, 0.0f);
		Expect(mixer, 0.25f);
		bad.Set(0.5f);
		Control(mixer, 1, invalid[i]);
		Expect(mixer, 0.25f);
		Control(mixer, 1, 1.0f);
		Expect(mixer, 0.75f);
	}
	// A bad sample does not suppress the rest of its block.
	bad.Set(3, invalid[0]);
	mixer.Execute();
	Sample *out = NULL;
	assert(mixer.GetOutput(0, &out));
	assert((*out)[2] == 0.75f * Scale && (*out)[3] == 0.25f * Scale);
	assert((*out)[4] == 0.75f * Scale);
	bad.Set(0.5f);
	Control(mixer, 1, -1.0f);
	Expect(mixer, -0.25f);
	// Preserve large and tiny finite values; this is not a limiter.
	good.Set(0.0f);
	bad.Set(1.0e20f);
	Control(mixer, 1, 1.0f);
	Expect(mixer, 1.0e20f);
	bad.Set(1.0e-20f);
	Expect(mixer, 1.0e-20f);
	// Finite operands can overflow when multiplied or accumulated.
	good.Set(0.25f);
	bad.Set(std::numeric_limits<float>::max());
	Control(mixer, 1, 2.0f);
	Expect(mixer, 0.25f);
	Control(mixer, 1, 1.0f);
	good.Set(std::numeric_limits<float>::max());
#ifdef TEST_STEREO
	Control(mixer, 0, 1.0f, Mixer::SETPAN);
	Control(mixer, 1, 1.0f, Mixer::SETPAN);
	mixer.Execute();
	assert(mixer.GetOutput(0, &out));
	assert((*out)[0] == std::numeric_limits<float>::max());
	assert(mixer.GetOutput(1, &out));
	assert((*out)[0] == 0.0f);
	Control(mixer, 0, 0.5f, Mixer::SETPAN);
	Control(mixer, 1, 0.5f, Mixer::SETPAN);
#else
	Expect(mixer, std::numeric_limits<float>::max());
#endif
	good.Set(0.25f);
	bad.Set(0.5f);
	Expect(mixer, 0.75f);
#ifdef TEST_STEREO
	Sample cv(8);
	assert(mixer.SetInput(5, &cv));
	for (int i = 0; i < 3; ++i)
	{
		cv.Set(invalid[i]);
		Expect(mixer, 0.75f);
		Control(mixer, 1, invalid[i], Mixer::SETPAN);
		Expect(mixer, 0.25f);
		Control(mixer, 1, 0.5f, Mixer::SETPAN);
	}
	cv.Set(2.0f); // Existing extrapolated pan remains valid.
	mixer.Execute();
	assert(mixer.GetOutput(0, &out));
	assert((*out)[0] == 0.875f);
	assert(mixer.GetOutput(1, &out));
	assert((*out)[0] == -0.125f);
	cv.Set(0.0f);
#endif
	// Patch representation and finite settings still round-trip.
	std::stringstream saved;
	mixer.StreamOut(saved);
	const std::string original = saved.str();
	Mixer restored;
	restored.Initialise(&host);
	restored.StreamIn(saved);
	std::stringstream roundtrip;
	restored.StreamOut(roundtrip);
	assert(roundtrip.str() == original);
}
