#include "Fl_Canvas.h"
#include "MixerPlugin.h"
#include "JackPlugin.h"
#include <dlfcn.h>
#include <cstdio>
#include <cassert>
#include <sstream>
#include <map>

class TestDevice : public SpiralPlugin
{
public:
	TestDevice()
	{
		m_PluginInfo.Name = "test";
		m_PluginInfo.Width = m_PluginInfo.Height = 50;
		m_PluginInfo.NumInputs = m_PluginInfo.NumOutputs = 1;
	}
	void Execute() {}
	void Apply(spiralcore::Description::Reader &) {}
	void Describe(spiralcore::Description &d) {}
};

static std::map<int, SpiralPlugin *> devices;
static std::map<int, Fl_DeviceGUI *> views;
static int disconnected = 0;

static DeviceGUIInfo Info(const PluginInfo &p)
{
	DeviceGUIInfo info;
	info.XPos = info.YPos = 0;
	info.Width = p.Width;
	info.Height = p.Height;
	info.NumInputs = p.NumInputs;
	info.NumOutputs = p.NumOutputs;
	info.Name = p.Name;
	info.PortTips = p.PortTips;
	info.PortTypes = p.PortTypes;
	return info;
}

static void Updated(int id, void *data)
{
	views[id]->SetupPorts(Info(*static_cast<PluginInfo *>(data)), false,
		dynamic_cast<StablePortLayout *>(devices[id]) != NULL);
}

static void Connect(Fl_Widget *, void *data)
{
	CanvasWire *wire = static_cast<CanvasWire *>(data);
	Sample *output = NULL;
	assert(devices[wire->OutputID]->GetOutput(wire->OutputPort, &output));
	assert(devices[wire->InputID]->SetInput(wire->InputPort, output));
}

static void Disconnect(Fl_Widget *, void *data)
{
	CanvasWire *wire = static_cast<CanvasWire *>(data);
	assert(devices[wire->InputID]->SetInput(wire->InputPort, NULL));
	++disconnected;
}

static unsigned WireCount(Fl_Canvas &canvas)
{
	std::stringstream stream;
	stream << canvas;
	int marker, version;
	unsigned count;
	stream >> marker >> version >> count;
	return count;
}

static void Command(SpiralPlugin &mixer, char command)
{
	mixer.GetChannelHandler()->SetCommand(command);
	mixer.UpdateChannelHandler();
	mixer.ExecuteCommands();
}

static void Check(SpiralPlugin &plugin, bool jack)
{
	HostInfo host = HostInfo();
	host.BUFSIZE = 8;
	host.SAMPLERATE = 44100;
	TestDevice source, sink, unrelatedSource, unrelatedSink;
	devices[0] = &source; devices[1] = &plugin; devices[2] = &sink;
	devices[3] = &unrelatedSource; devices[4] = &unrelatedSink;
	Fl_Canvas canvas(0, 0, 600, 400, "test");
	canvas.end();
	canvas.SetConnectionCallback(Connect);
	canvas.SetUnconnectCallback(Disconnect);
	for (int id = 0; id < 5; ++id)
	{
		devices[id]->Initialise(&host);
		views[id] = new Fl_DeviceGUI(Info(devices[id]->GetPluginInfo()), NULL, NULL);
		views[id]->end();
		views[id]->SetID(id);
		canvas.add(views[id]);
		devices[id]->SetUpdateInfoCallback(id, Updated);
	}
	assert(dynamic_cast<StablePortLayout *>(&plugin));
	if (jack)
	{
		plugin.GetChannelHandler()->Set("NumInputs", 4);
		plugin.GetChannelHandler()->Set("NumOutputs", 4);
		Command(plugin, JackPlugin::SET_PORT_COUNT);
	}
	std::stringstream wires("-1 0 4\n0 0 0 0 1 0 0 0\n0 0 0 0 1 0 3 0\n1 0 0 0 2 0 0 1\n3 0 0 0 4 0 0 1\n");
	canvas.StreamWiresIn(wires, false, false);
	assert(WireCount(canvas) == 4);
	const Sample *input = plugin.GetInput(0);
	const Sample *output = sink.GetInput(0);
	for (int cycle = 0; cycle < 3; ++cycle)
	{
		if (jack)
		{
			plugin.GetChannelHandler()->Set("NumInputs", 3);
			plugin.GetChannelHandler()->Set("NumOutputs", 3);
			Command(plugin, JackPlugin::SET_PORT_COUNT);
		}
		else Command(plugin, MixerPlugin::REMOVECHAN);
		assert(WireCount(canvas) == 3);
		assert(plugin.GetInput(0) == input && sink.GetInput(0) == output);
		if (jack)
		{
			plugin.GetChannelHandler()->Set("NumInputs", 4);
			plugin.GetChannelHandler()->Set("NumOutputs", 4);
			Command(plugin, JackPlugin::SET_PORT_COUNT);
		}
		else Command(plugin, MixerPlugin::ADDCHAN);
		assert(WireCount(canvas) == 3);
		assert(plugin.GetInput(0) == input && sink.GetInput(0) == output);
	}
	plugin.SetUpdateInfoCallback(0, NULL);
}

int main(int argc, char **argv)
{
	if (argc != 3) return 77;
	for (int n = 1; n < argc; ++n)
	{
		void *module = dlopen(argv[n], RTLD_NOW | RTLD_LOCAL);
		if (!module) { puts(dlerror()); return 1; }
		SpiralPlugin *(*create)() = (SpiralPlugin *(*)())dlsym(module, "SpiralPlugin_CreateInstance");
		assert(create);
		SpiralPlugin *plugin = create();
		Check(*plugin, n == 2);
		delete plugin;
	}
}
