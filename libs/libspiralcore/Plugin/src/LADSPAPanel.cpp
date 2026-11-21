/*  SpiralSynthModular
 *  Copyleft (C) 2026 ajgenius
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "LADSPAPlugin.h"
#include "JSON.h"
#include "PanelFiles.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <unistd.h>

using namespace std;
using Spumoni::JSON;

namespace
{
	const int FrameX = 2, FrameY = 14;
	const int Pages = 3;
	enum { KnobsPage, SlidersPage, SetupPage };

	JSON *Number(double value)
	{
		char text[32];
		snprintf(text, sizeof(text), "%.9g", value);
		return JSON::MakeNumber(text);
	}

	JSON *Text(const string &text) { return JSON::MakeString(text); }

	JSON *Bounds(int x, int y, int w, int h)
	{
		JSON *bounds = JSON::MakeArray();
		bounds->AppendOwned(Number(x - FrameX));
		bounds->AppendOwned(Number(y - FrameY));
		bounds->AppendOwned(Number(w));
		bounds->AppendOwned(Number(h));
		return bounds;
	}

	string Indexed(const char *base, unsigned long index)
	{
		ostringstream name;
		name << base << index;
		return name.str();
	}

	JSON *Node(const string &type, const string &name, int x, int y, int w, int h, bool inContent = false)
	{
		JSON *node = JSON::MakeObject();
		const string prefix = type == "LCD" ? "Meters::" : type == "LEDButton" ? "Basic::" : "";
		node->SetOwned("type", Text(prefix + type));
		node->SetOwned("name", Text(name));
		node->SetOwned("bounds", inContent ? Bounds(x + FrameX, y + FrameY, w, h) : Bounds(x, y, w, h));
		return node;
	}

	JSON *Label(const string &name, int x, int y, int w, int h, const string &text, int size, const char *align = NULL, bool inContent = false)
	{
		JSON *node = Node("Label", name, x, y, w, h, inContent);
		node->SetOwned("text", Text(text));
		node->SetOwned("size", Number(size));
		if (align)
			node->SetOwned("align", Text(align));
		return node;
	}

	JSON *PortLabel(const string &name, int x, int y, int w, const char *port, bool slider)
	{
		size_t len = strlen(port);
		len = len > 24 ? 19 : len >= 5 ? len - 5 : len;
		if (slider && len > 11)
			len = 11;
		JSON *node = Label(name, x, y, w, 15, string(port, len), 9);
		node->SetOwned("clip", JSON::MakeBoolean(true));
		return node;
	}

	JSON *Property(const string &name, const string &type)
	{
		JSON *node = JSON::MakeObject();
		node->SetOwned("property", Text(name));
		node->SetOwned("type", Text(type));
		return node;
	}

	JSON *PortProperty(const string &base, unsigned long index, const string &type, bool readOnly = false)
	{
		JSON *node = Property(Indexed(base.c_str(), index), type);
		if (readOnly)
			node->SetOwned("read_only", JSON::MakeBoolean(true));
		return node;
	}

	void Writes(JSON *node, const char *command, long index = -1)
	{
		JSON *write = JSON::MakeObject();
		write->SetOwned("write", Text("COMMAND"));
		write->SetOwned("value", Text(command));
		JSON *with = JSON::MakeArray();
		if (index >= 0)
			with->AppendOwned(Number((double)index));
		with->AppendOwned(Text("value"));
		write->SetOwned("with", with);
		JSON *writes = JSON::MakeArray();
		writes->AppendOwned(write);
		JSON *handlers = JSON::MakeObject();
		handlers->SetOwned("value_changed", writes);
		node->SetOwned("on", handlers);
	}

	void Locked(JSON *node, unsigned long index)
	{
		JSON *when = JSON::MakeObject();
		when->SetOwned("property", Text(Indexed("CONNECTED", index)));
		when->SetOwned("equals", Number(0));
		node->SetOwned("enabled", when);
	}

	void Range(JSON *node, const PortSetting &setting, unsigned long index)
	{
		JSON *min = JSON::MakeObject();
		min->SetOwned("property", Text(Indexed("MIN", index)));
		JSON *max = JSON::MakeObject();
		max->SetOwned("property", Text(Indexed("MAX", index)));
		node->SetOwned("min", min);
		node->SetOwned("max", max);
		if (setting.Integer)
			node->SetOwned("step", Number(1));
		else if (setting.LogBase > 1.0f)
		{
			JSON *mapping = JSON::MakeObject();
			mapping->SetOwned("kind", Text("symmetric_log"));
			mapping->SetOwned("base", Number(setting.LogBase));
			node->SetOwned("mapping", mapping);
		}
	}

	JSON *PageButton(const string &name, int x, int y, const string &text, int page)
	{
		JSON *node = Node("Button", name, x, y, 50, 20);
		node->SetOwned("text", Text(text));
		node->SetOwned("property", Text("PAGE"));
		node->SetOwned("choice", Number(page));
		JSON *other = JSON::MakeObject();
		other->SetOwned("property", Text("PAGE"));
		other->SetOwned("not_equals", Number(page));
		node->SetOwned("enabled", other);
		return node;
	}

	void KnobGrid(unsigned long count, int &cols, int &rows, int &width, int &height)
	{
		float sqrcount = sqrtf((float)count);
		float fsqrcount = floorf(sqrcount);
		float diff = sqrcount - fsqrcount;
		cols = (int)fsqrcount;
		rows = (int)fsqrcount;
		rows += diff > 0.0f ? 1 : 0;
		cols += diff > 0.5f ? 1 : 0;
		if (count == 0)
		{
			width = 170;
			height = 80;
		}
		else if (count < 3)
		{
			cols = (int)count;
			rows = 1;
			width = 10 + (int)count * 100;
			height = 45 + 80;
		}
		else
		{
			width = 10 + cols * 100;
			height = 45 + rows * 80;
		}
		if (width < 170)
			width = 170;
	}

	void SliderGrid(unsigned long count, int &cols, int &rows, int &fullrows, int &width, int &height)
	{
		if (count < 9)
		{
			cols = (int)count;
			rows = 1;
			fullrows = 1;
		}
		else
		{
			float sqrcount = sqrtf((float)count);
			cols = (int)floorf(sqrcount * 2.0f);
			rows = (int)floorf(sqrcount * 0.5f);
			float aspect = (float)cols / (float)rows;
			int diff = cols * rows - (int)count;
			if (diff < 0)
			{
				if (aspect > 4.0f)
				{
					rows++;
					diff += cols;
				}
				if (diff > rows - 1)
					cols -= (int)floorf((float)diff / (float)rows);
				else if (diff < 0)
					cols += (int)ceilf(fabsf((float)diff) / (float)rows);
			}
			fullrows = rows - (cols * rows - (int)count);
		}
		if (cols < 1)
			cols = 1;
		if (count == 0)
		{
			width = 170;
			height = 80;
		}
		else if (count < 3)
		{
			width = 170;
			height = 45 + 140;
		}
		else if (count < 9)
		{
			width = 10 + (int)count * 60;
			height = 45 + 140;
		}
		else
		{
			width = 10 + cols * 60;
			height = 45 + rows * 140;
		}
		if (width < 170)
			width = 170;
	}
}

// The switch between the generic pages and the plugin's own panel.
static JSON *Modes()
{
	JSON *modes = JSON::MakeArray();
	const char *names[] = {"Generic", "Panel"};

	for (int mode = 0; mode < 2; ++mode)
	{
		JSON *entry = JSON::MakeObject();
		entry->SetOwned("name", Text(names[mode]));
		entry->SetOwned("property", Text("PANELMODE"));
		entry->SetOwned("value", Number(mode));
		modes->AppendOwned(entry);
	}

	return modes;
}

unsigned LADSPAPlugin::PanelGeneration()
{
	int generation = 0;
	m_AudioCH->GetData("GetPanelGeneration", &generation);
	return (unsigned)generation;
}

// The panel a file in the panels folders has for the loaded effect.
static JSON *FindPanel(const string &directory, unsigned uniqueID)
{
	return Spiral::Core::PanelFiles(Spiral::Core::PanelFiles::Folders(directory)).Read("ladspa", uniqueID);
}

bool LADSPAPlugin::GeneratePanel(const string &Directory, string &Document)
{
	unsigned long count = 0;
	int page = SetupPage, mode = 0;
	unsigned uniqueID = 0;
	char name[256] = {0}, maker[256] = {0};
	m_AudioCH->GetData("Ports", &count);
	m_AudioCH->Peek("Page", 0, &page, sizeof(page));
	m_AudioCH->Peek("PanelMode", 0, &mode, sizeof(mode));
	m_AudioCH->Peek("UniqueID", 0, &uniqueID, m_AudioCH->ChannelSize("UniqueID"));
	m_AudioCH->GetData("PluginName", name);
	m_AudioCH->GetData("Maker", maker);
	if (count > m_MaxInputPortCount)
		count = m_MaxInputPortCount;
	vector<char> names(256 * m_MaxInputPortCount);
	vector<PortSetting> settings(m_MaxInputPortCount);
	vector<char> connected(m_MaxInputPortCount);
	m_AudioCH->GetData("PortName", &names[0]);
	m_AudioCH->GetData("GetInputPortSettings", &settings[0]);
	m_AudioCH->GetData("Connected", &connected[0]);

	Spumoni::JSONOwner custom(uniqueID ? FindPanel(Directory, uniqueID) : NULL);
	if (page < 0 || page >= Pages)
		page = SetupPage;
	const bool own = custom.get() && mode == 1 && custom->Get("panel") && custom->Get("panel")->GetType() == JSON::Object;

	JSON *root = JSON::MakeObject();
	root->SetOwned("schema_version", Number(1));
	root->SetOwned("plugin_id", Number(Class().ID));
	JSON *contracts = JSON::MakeObject();
	contracts->SetOwned("Controls", Number(1));
	contracts->SetOwned("Basic", Number(1));
	contracts->SetOwned("Meters", Number(1));
	root->SetOwned("control_contracts", contracts);
	root->SetOwned("generated", JSON::MakeBoolean(true));

	JSON *properties = JSON::MakeArray();
	{
		JSON *command = Property("COMMAND", "string");
		command->SetOwned("internal", JSON::MakeBoolean(true));
		properties->AppendOwned(command);
		JSON *unique = Property("UNIQUEID", "integer");
		unique->SetOwned("read_only", JSON::MakeBoolean(true));
		properties->AppendOwned(unique);
		properties->AppendOwned(Property("PAGE", "integer"));
		JSON *panelMode = Property("PANELMODE", "integer");
		panelMode->SetOwned("internal", JSON::MakeBoolean(true));
		properties->AppendOwned(panelMode);
		properties->AppendOwned(Property("UPDATEINPUTS", "boolean"));
		JSON *pluginName = Property("PLUGINNAME", "string");
		pluginName->SetOwned("read_only", JSON::MakeBoolean(true));
		properties->AppendOwned(pluginName);
		JSON *pluginMaker = Property("MAKER", "string");
		pluginMaker->SetOwned("read_only", JSON::MakeBoolean(true));
		properties->AppendOwned(pluginMaker);
	}
	for (unsigned long p = 0; p < count; p++)
	{
		properties->AppendOwned(PortProperty("DEFAULT", p, "number", true));
		properties->AppendOwned(PortProperty("MIN", p, "number", true));
		properties->AppendOwned(PortProperty("MAX", p, "number", true));
		properties->AppendOwned(PortProperty("CLAMP", p, "boolean", true));
		properties->AppendOwned(PortProperty("VALUE", p, "number", true));
		properties->AppendOwned(PortProperty("CONNECTED", p, "boolean", true));
		properties->AppendOwned(PortProperty("ENABLED", p, "boolean", true));
	}
	root->SetOwned("properties", properties);

	JSON *children = JSON::MakeArray();
	int width = 500, height = 320;
	unsigned long unconnected = 0;
	for (unsigned long p = 0; p < count; p++)
		if (!connected[p])
			unconnected++;

	// The plugin's own panel is shown as it was written, at its own size,
	// with the switch back to the generic pages.
	if (own)
	{
		JSON *panel = custom->Get("panel")->Duplicate();
		panel->SetOwned("modes", Modes());
		root->SetOwned("panel", panel);
		delete children;
		Document = root->Stringify(false);
		delete root;
		return true;
	}

	if (page == KnobsPage)
	{
		int cols, rows;
		KnobGrid(unconnected, cols, rows, width, height);
		int column = 0, row = 0;
		for (unsigned long p = 0; p < count; p++)
		{
			if (connected[p])
				continue;
			int kx, dx, lx, y = 45 + row * 80;
			if (unconnected == 1)
			{
				kx = 35 + 30; dx = 35 + 20; lx = 5 + 30;
			}
			else
			{
				kx = 35 + column * 100; dx = 25 + column * 100; lx = 5 + column * 100;
			}
			JSON *knob = Node("Knob", Indexed("knob-", p), kx, y, 40, 40);
			knob->SetOwned("property", Text(Indexed("DEFAULT", p)));
			Writes(knob, "SETDEFAULT", (long)p);
			Locked(knob, p);
			Range(knob, settings[p], p);
			children->AppendOwned(knob);
			JSON *entry = Node("TextEntry", Indexed("knob-value-", p), dx, y + 40, 60, 16);
			entry->SetOwned("property", Text(Indexed("DEFAULT", p)));
			Writes(entry, "SETDEFAULT", (long)p);
			Locked(entry, p);
			entry->SetOwned("format", Text("%.4f"));
			children->AppendOwned(entry);
			children->AppendOwned(PortLabel(Indexed("knob-label-", p), lx, y + 55, 100, &names[p * 256], false));
			if (++column == cols)
			{
				column = 0;
				row++;
			}
		}
	}
	else if (page == SlidersPage)
	{
		int cols, rows, fullrows;
		SliderGrid(unconnected, cols, rows, fullrows, width, height);
		int column = 0, row = 0;
		for (unsigned long p = 0; p < count; p++)
		{
			if (connected[p])
				continue;
			const int offset = unconnected < 3 ? (160 - (int)unconnected * 60) / 2 : 0;
			int sx = 25 + offset + column * 60, y = 45 + row * 140;
			JSON *slider = Node("Slider", Indexed("slider-", p), sx, y, 20, 100);
			slider->SetOwned("property", Text(Indexed("DEFAULT", p)));
			Writes(slider, "SETDEFAULT", (long)p);
			Locked(slider, p);
			slider->SetOwned("vertical", JSON::MakeBoolean(true));
			Range(slider, settings[p], p);
			children->AppendOwned(slider);
			JSON *entry = Node("TextEntry", Indexed("slider-value-", p), sx - 18, y + 101, 56, 16);
			entry->SetOwned("property", Text(Indexed("DEFAULT", p)));
			Writes(entry, "SETDEFAULT", (long)p);
			Locked(entry, p);
			entry->SetOwned("format", Text("%.4f"));
			children->AppendOwned(entry);
			children->AppendOwned(PortLabel(Indexed("slider-label-", p), sx - 20, y + 116, 60, &names[p * 256], true));
			if (++column == cols - (row < fullrows ? 0 : 1))
			{
				column = 0;
				row++;
			}
		}
	}
	else if (page == SetupPage)
	{
		children->AppendOwned(Label("name", 10, 45, 480, 15, name, 12, "left"));
		children->AppendOwned(Label("maker", 10, 65, 480, 15, maker, 12, "left"));
		children->AppendOwned(Label("plugin-label", 10, 90, 40, 22, "Plugin:", 12, "right"));
		JSON *chooser = Node("Choice", "plugin", 50, 90, 440, 22);
		chooser->SetOwned("property", Text("UNIQUEID"));
		Writes(chooser, "SELECTPLUGIN");
		chooser->SetOwned("size", Number(12));
		JSON *items = JSON::MakeArray();
		{
			JSON *none = JSON::MakeObject();
			none->SetOwned("text", Text("(None)"));
			none->SetOwned("value", Number(0));
			items->AppendOwned(none);
		}
		const vector<LADSPAInfo::PluginEntry> &menu = m_LADSPAInfo->GetMenuList();
		for (vector<LADSPAInfo::PluginEntry>::const_iterator i = menu.begin(); i != menu.end(); ++i)
		{
			JSON *item = JSON::MakeObject();
			item->SetOwned("text", Text(i->Name));
			item->SetOwned("value", Number((double)i->UniqueID));
			items->AppendOwned(item);
		}
		chooser->SetOwned("items", items);
		children->AppendOwned(chooser);

		children->AppendOwned(Label("value-label", 15, 120, 60, 15, "Value", 12, "left"));
		children->AppendOwned(Label("default-label", 77, 120, 60, 15, "Default", 12, "left"));
		children->AppendOwned(Label("min-label", 139, 120, 60, 15, "Min", 12, "left"));
		children->AppendOwned(Label("max-label", 201, 120, 60, 15, "Max", 12, "left"));
		children->AppendOwned(Label("clamp-label", 275, 120, 60, 15, "Clamp?", 12, "left"));
		children->AppendOwned(Label("port-label", 335, 120, 60, 15, "Port Name", 12, "left"));
		children->AppendOwned(Label("wire-label", 445, 120, 40, 15, "Wire?", 10, "left"));

		JSON *rows = JSON::MakeArray();
		for (unsigned long p = 0; p < count; p++)
		{
			const int y = 5 + (int)p * 24;
			JSON *value = Node("LCD", Indexed("value-", p), 5, y, 60, 18, true);
			value->SetOwned("property", Text(Indexed("VALUE", p)));
			value->SetOwned("format", Text("%.4f"));
			value->SetOwned("size", Number(10));
			rows->AppendOwned(value);
			JSON *fallback = Node("TextEntry", Indexed("default-", p), 67, y, 60, 18, true);
			fallback->SetOwned("property", Text(Indexed("DEFAULT", p)));
			Writes(fallback, "SETDEFAULT", (long)p);
			Locked(fallback, p);
			fallback->SetOwned("format", Text("%.4f"));
			JSON *unwired = JSON::MakeObject();
			unwired->SetOwned("property", Text(Indexed("CONNECTED", p)));
			unwired->SetOwned("equals", Number(0));
			fallback->SetOwned("enabled", unwired);
			rows->AppendOwned(fallback);
			JSON *min = Node("TextEntry", Indexed("min-", p), 129, y, 60, 18, true);
			min->SetOwned("property", Text(Indexed("MIN", p)));
			Writes(min, "SETMIN", (long)p);
			min->SetOwned("format", Text("%.4f"));
			rows->AppendOwned(min);
			JSON *max = Node("TextEntry", Indexed("max-", p), 191, y, 60, 18, true);
			max->SetOwned("property", Text(Indexed("MAX", p)));
			Writes(max, "SETMAX", (long)p);
			max->SetOwned("format", Text("%.4f"));
			rows->AppendOwned(max);
			JSON *clamp = Node("CheckBox", Indexed("clamp-", p), 260, y, 18, 18, true);
			clamp->SetOwned("property", Text(Indexed("CLAMP", p)));
			Writes(clamp, "SETCLAMP", (long)p);
			clamp->SetOwned("size", Number(14));
			rows->AppendOwned(clamp);
			rows->AppendOwned(Label(Indexed("port-name-", p), 320, y, 112, 18, &names[p * 256], 10, "left", true));
			JSON *wire = Node("CheckBox", Indexed("wire-", p), 437, y, 18, 18, true);
			wire->SetOwned("property", Text(Indexed("ENABLED", p)));
			Writes(wire, "SETPORTENABLED", (long)p);
			Locked(wire, p);
			wire->SetOwned("size", Number(14));
			rows->AppendOwned(wire);
		}
		JSON *scroll = Node("VirtualScrollArea", "inputs", 10, 135, 480, 145);
		scroll->SetOwned("children", rows);
		scroll->SetOwned("show_horizontal", JSON::MakeBoolean(false));
		JSON *content = JSON::MakeArray();
		content->AppendOwned(Number(455));
		content->AppendOwned(Number(5 + (int)count * 24));
		scroll->SetOwned("content_size", content);
		children->AppendOwned(scroll);

		JSON *update = Node("LEDButton", "update-inputs", 10, 282, 25, 25);
		update->SetOwned("property", Text("UPDATEINPUTS"));
		update->SetOwned("text", Text("Update input values?"));
		update->SetOwned("size", Number(12));
		children->AppendOwned(update);
	}

	children->AppendOwned(PageButton("knobs", 5, 15, "Knobs", KnobsPage));
	children->AppendOwned(PageButton("sliders", 60, 15, "Sliders", SlidersPage));
	children->AppendOwned(PageButton("setup", width - 55, 15, "Setup...", SetupPage));

	JSON *panel = JSON::MakeObject();
	panel->SetOwned("schema_version", Number(1));
	panel->SetOwned("width", Number(width - 2 * FrameX));
	panel->SetOwned("height", Number(height - FrameY - FrameX));
	panel->SetOwned("children", children);
	if (custom.get())
		panel->SetOwned("modes", Modes());
	panel->SetOwned("help", Text("A LADSPA plugin: pick one on the Setup page, where each input has its "
		"default (used while no wire feeds it), its range and whether a wire's "
		"signal is clamped to that range. Knobs and Sliders show the unwired "
		"inputs' defaults to turn. A plugin with a panel written for it has a switch between that and these pages."));
	root->SetOwned("panel", panel);

	Document = root->Stringify(false);
	delete root;
	return true;
}
