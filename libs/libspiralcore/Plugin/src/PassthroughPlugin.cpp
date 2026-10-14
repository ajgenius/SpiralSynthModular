/*  SpiralSound
 *  Copyleft (C) 2001 David Griffiths <dave@pawfal.org>
 *  Passthrough device from the Scope plugin: GROK (Grok Bot), 2026
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include "PassthroughPlugin.h"
#include "OutputIcon.xpm"
#include <fstream>
#include <iostream>
#include <list>

using namespace std;
using namespace Spiral::Core;

namespace {
struct Pixmap
{
	vector<string> Lines;
	vector<const char *> Rows;
};
}

PassthroughPlugin::PassthroughPlugin(const PluginManifest &manifest) :
m_Pairs(static_cast<int>(manifest.ports.size())),
m_StateVersion(manifest.stateVersion),
m_Held(manifest.properties.size())
{
	m_PluginInfo.Name = manifest.name;
	m_PluginInfo.Width = manifest.width;
	m_PluginInfo.Height = manifest.height;
	m_PluginInfo.NumInputs = m_Pairs;
	m_PluginInfo.NumOutputs = m_Pairs;
	for (int i = 0; i < m_Pairs; ++i)
		m_PluginInfo.PortTips.push_back(manifest.ports[i].input);
	for (int i = 0; i < m_Pairs; ++i)
		m_PluginInfo.PortTips.push_back(manifest.ports[i].output);

	SetString(DeviceName, manifest.name);
	SetUnsigned(DeviceVersion, m_StateVersion);
	for (size_t i = 0; i < m_Held.size(); ++i)
	{
		Held &held = m_Held[i];
		const PluginManifest::Property &p = manifest.properties[i];
		held.Manifest = p;
		if (p.type == "toggle")
			held.Property = new ToggleProperty(Property::WriteOnly, p.value != 0);
		else if (p.type == "unsigned")
			held.Property = new UnsignedProperty(DefaultLinearFlags, (UnsignedType)p.value, (UnsignedType)p.min, (UnsignedType)p.max, (UnsignedType)(p.step > 0 ? p.step : 1), (UnsignedType)(p.step > 0 ? p.step : 1));
		else
			held.Property = new FloatProperty(DefaultLinearFlags, (FloatType)p.value, (FloatType)p.min, (FloatType)p.max, (FloatType)p.step, (FloatType)p.step);
		RegisterSharedProperty(*held.Property, p.name, p.hint);
	}
}

PassthroughPlugin::~PassthroughPlugin()
{
	for (size_t i = 0; i < m_Held.size(); ++i)
		delete m_Held[i].Property;
}

void PassthroughPlugin::Execute()
{
	// Just copy the data through.
	for (int i = 0; i < m_Pairs; ++i)
	{
		if (GetOutputBuf(i))
			GetOutputBuf(i)->Zero();
		if (InputExists(i) && GetOutputBuf(i))
			GetOutputBuf(i)->Mix(*GetInput(i), 0);
	}
}

void PassthroughPlugin::Describe(Description &d)
{
	if (m_StateVersion)
		d.Value(DeviceVersion).Separator(" ");
	for (size_t i = 0; i < m_Held.size(); ++i)
	{
		const Held &held = m_Held[i];
		if (held.Manifest.type == "toggle")
			d.Value(*(ToggleProperty *)held.Property);
		else if (held.Manifest.type == "unsigned")
			d.Value(*(UnsignedProperty *)held.Property);
		else
			d.Value(*(FloatProperty *)held.Property);
		d.Separator(" ");
	}
}

void PassthroughPlugin::Apply(Description::Reader &r)
{
	if (m_StateVersion)
	{
		int version = 0;
		if (!r.More() || r.Value(version).Failed() || version != m_StateVersion)
			return;
	}
	for (size_t i = 0; i < m_Held.size(); ++i)
	{
		Held &held = m_Held[i];
		if (held.Manifest.type == "toggle")
			r.Value(*(ToggleProperty *)held.Property);
		else if (held.Manifest.type == "unsigned")
			r.Value(*(UnsignedProperty *)held.Property);
		else
			r.Value(*(FloatProperty *)held.Property);
	}
}

bool PassthroughPlugin::ReadClass(const string &manifest, DeviceClass &device)
{
	PluginManifest read;
	string error;
	if (!read.Read(manifest, error) || read.registration != "builtin" || read.builtin != "passthrough")
		return false;
	device.ID = read.id;
	device.Name = read.name;
	device.Category = read.category;
	device.Manifest = manifest;
	device.Build = Build;
	device.Icon = Icon(device.Directory());
	return true;
}

SpiralPlugin *PassthroughPlugin::Build(const DeviceClass &device)
{
	PluginManifest read;
	string error;
	if (!read.Read(device.Manifest, error) || read.builtin != "passthrough")
	{
		cerr << device.Manifest << ": " << error << endl;
		return NULL;
	}
	return new PassthroughPlugin(read);
}

const char **PassthroughPlugin::Icon(const string &directory)
{
	static list<Pixmap> icons;

	ifstream file((directory + "/SpiralIcon.xpm").c_str());
	Pixmap icon;
	string line;
	while (getline(file, line))
	{
		string::size_type open = line.find('"'), close = line.rfind('"');
		if (open != string::npos && close > open)
			icon.Lines.push_back(line.substr(open + 1, close - open - 1));
	}
	if (icon.Lines.empty())
		return SpiralIcon_xpm;
	icons.push_back(icon);
	Pixmap &kept = icons.back();
	for (size_t i = 0; i < kept.Lines.size(); ++i)
		kept.Rows.push_back(kept.Lines[i].c_str());
	return &kept.Rows[0];
}
