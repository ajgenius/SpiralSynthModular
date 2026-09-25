#include "Fl_Canvas.h"
#include "MixerPlugin.h"
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
	void StreamIn(std::istream &) {}
	void StreamOut(std::ostream &) {}
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

static void Command(MixerPlugin &mixer, char command)
{
	mixer.GetChannelHandler()->SetCommand(command);
	mixer.UpdateChannelHandler();
	mixer.ExecuteCommands();
}

int main()
{
	HostInfo host = HostInfo();
	host.BUFSIZE = 8;
	MixerPlugin mixer;
	TestDevice source, sink, unrelatedSource, unrelatedSink;
	devices[0] = &source; devices[1] = &mixer; devices[2] = &sink;
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
	std::stringstream wires("-1 0 4\n0 0 0 0 1 0 0 0\n0 0 0 0 1 0 3 0\n1 0 0 0 2 0 0 1\n3 0 0 0 4 0 0 1\n");
	canvas.StreamWiresIn(wires, false, false);
	assert(WireCount(canvas) == 4);
	const Sample *input = mixer.GetInput(0);
	const Sample *output = sink.GetInput(0);
	const Sample *unrelated = unrelatedSink.GetInput(0);
	Command(mixer, MixerPlugin::REMOVECHAN);
	assert(mixer.GetChannels() == 3 && WireCount(canvas) == 3 && disconnected == 1);
	assert(mixer.GetInput(0) == input && sink.GetInput(0) == output);
	assert(unrelatedSink.GetInput(0) == unrelated);
	Command(mixer, MixerPlugin::REMOVECHAN);
	assert(mixer.GetChannels() == 2 && WireCount(canvas) == 3 && disconnected == 1);
	Command(mixer, MixerPlugin::REMOVECHAN);
	assert(mixer.GetChannels() == 2);
	for (int n = 0; n < 20; ++n) Command(mixer, MixerPlugin::ADDCHAN);
	assert(mixer.GetChannels() == MAX_CHANNELS && WireCount(canvas) == 3);
	assert(mixer.GetInput(0) == input && sink.GetInput(0) == output);
	// Rebuilt port buttons must retain connection counts as well as wires.
	canvas.PortClicked(views[1], Fl_DeviceGUI::OUTPUT, 0, false);
	assert(WireCount(canvas) == 2 && disconnected == 2);
	assert(sink.GetInput(0) == NULL && unrelatedSink.GetInput(0) == unrelated);
	canvas.PortClicked(views[0], Fl_DeviceGUI::OUTPUT, 0, false);
	assert(WireCount(canvas) == 1 && mixer.GetInput(0) == NULL);
}
