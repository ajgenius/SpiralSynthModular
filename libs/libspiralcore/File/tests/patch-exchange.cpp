// SPDX-License-Identifier: GPL-2.0-or-later
#include "FileVersion.h"
#include "PatchFile.h"
#include "JSON.h"
#include <cassert>

using namespace Spiral::Core;
using Spiral::File::PatchFile;

static DeviceDescription &AddDevice(PatchDescription &patch, const char *id, const char *key)
{
	DeviceDescription &device = patch.AddObject().AddDevice();
	device.SetValue("id", "4096");
	device.SetValue("patch_id", id);
	device.SetValue("name", id);
	if (*key)
		device.SetValue("key", key);
	device.Outputs.push_back(PortName("Output", "AudioPort"));
	device.AddProperty("PRESET", "string", "StringProperty").SetValue("my preset");
	return device;
}

int main()
{
	PatchDescription patch;
	AddDevice(patch, "legacy", "");
	AddDevice(patch, "left", "org.example.osc");
	AddDevice(patch, "right", "org.example.osc");
	DeviceDescription &bus = patch.AddObject().AddDevice();
	bus.SetValue("patch_id", "bus");
	bus.SetValue("name", "SubPatch");
	PatchDescription &inner = bus.MakeSubPatch();
	inner.Document.Title = "Inner take";
	AddDevice(inner, "effect", "org.example.delay");
	bus.Inputs.push_back(PortName("Return", "AudioPort"));
	WireDescription &wire = patch.AddWire();
	wire.FromDevice = "left";
	wire.FromPort = wire.ToPort = "0";
	wire.ToDevice = "bus";
	wire.FromPortName = "Output";
	wire.ToPortName = "Return";

	std::string text, error;
	assert(PatchFile::Text(patch, text, error));
	PatchDescription reloaded;
	assert(PatchFile::ReadText(text, reloaded, error));
	assert(reloaded.FindDevice("legacy")->Value("id") == "4096");
	assert(reloaded.FindDevice("left")->Value("id") != "4096");
	assert(reloaded.FindDevice("left")->Value("id") == reloaded.FindDevice("right")->Value("id"));
	assert(reloaded.FindDevice("right")->Value("key") == "org.example.osc");
	const PatchDescription &child = *reloaded.FindDevice("bus")->Inner();
	assert(child.Document.Title == "Inner take" && child.FindDevice("effect")->Value("id") == "4096");
	assert(reloaded.Wires.front().ToPortName == "Return");
	const std::string schema = PatchFile::Schema(reloaded);
	reloaded.Objects[1].Devices[0].Outputs.clear();
	PatchFile::ApplySchema(schema, reloaded);
	assert(reloaded.FindDevice("left")->Outputs.front() == PortName("Output", "AudioPort"));

	Spumoni::JSONOwner document(Spumoni::ParseJSONText(text));
	document->SetOwned("format", Spumoni::JSON::MakeString("SpiralSynthModular Patch File Ver 5"));
	assert(PatchFile::Read(*document.get(), reloaded, error));
	document->SetOwned("format", Spumoni::JSON::MakeString("SpiralSynthModular Patch File Ver 6"));
	assert(!PatchFile::Read(*document.get(), reloaded, error));
	assert(reloaded.FindDevice("left") && !error.empty());
	return 0;
}
