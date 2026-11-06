#ifdef TEST_STEREO
#include "StereoMixerPlugin.h"
typedef StereoMixerPlugin Mixer;
static const float Scale = 0.5f;
#else
#include "MixerPlugin.h"
typedef MixerPlugin Mixer;
static const float Scale = 1.0f;
#endif
#include <cassert>
#include <limits>
#include <sstream>

static void Control(Mixer &mixer, int channel, float value, const char *what = "Volume")
{
	ChannelHandler *ch = mixer.GetChannelHandler();
	char name[32];
	snprintf(name, sizeof name, "%s%d", what, channel);
	ch->Set(name, value);
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
	bad.Set(3, invalid[0]);
	mixer.Execute();
	Sample *out = NULL;
	assert(mixer.GetOutput(0, &out));
	assert((*out)[2] == 0.75f * Scale && (*out)[3] == 0.25f * Scale);
	assert((*out)[4] == 0.75f * Scale);
	bad.Set(0.5f);
	Control(mixer, 1, -1.0f);
	Expect(mixer, -0.25f);
	good.Set(0.0f);
	bad.Set(1.0e20f);
	Control(mixer, 1, 1.0f);
	Expect(mixer, 1.0e20f);
	bad.Set(1.0e-20f);
	Expect(mixer, 1.0e-20f);
	good.Set(0.25f);
	bad.Set(std::numeric_limits<float>::max());
	Control(mixer, 1, 2.0f);
	Expect(mixer, 0.25f);
	Control(mixer, 1, 1.0f);
	good.Set(std::numeric_limits<float>::max());
#ifdef TEST_STEREO
	Control(mixer, 0, 1.0f, "Pan");
	Control(mixer, 1, 1.0f, "Pan");
	mixer.Execute();
	assert(mixer.GetOutput(0, &out));
	assert((*out)[0] == std::numeric_limits<float>::max());
	assert(mixer.GetOutput(1, &out));
	assert((*out)[0] == 0.0f);
	Control(mixer, 0, 0.5f, "Pan");
	Control(mixer, 1, 0.5f, "Pan");
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
		Control(mixer, 1, invalid[i], "Pan");
		Expect(mixer, 0.25f);
		Control(mixer, 1, 0.5f, "Pan");
	}
	cv.Set(2.0f);
	mixer.Execute();
	assert(mixer.GetOutput(0, &out));
	assert((*out)[0] == 0.875f);
	assert(mixer.GetOutput(1, &out));
	assert((*out)[0] == -0.125f);
	cv.Set(0.0f);
#endif
	Spiral::Core::Description saved;
	mixer.Describe(saved);
	Mixer restored;
	restored.Initialise(&host);
	Spiral::Core::Description::Reader reader(saved);
	restored.Apply(reader);
	assert(!reader.Failed() && (reader.Named() || !reader.More()));
	Spiral::Core::Description roundtrip;
	restored.Describe(roundtrip);
	assert(roundtrip.Values() == saved.Values());
}
