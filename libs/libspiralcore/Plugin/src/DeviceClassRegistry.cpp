/*  SpiralSound
 *  Copyleft (C) 2002 David Griffiths <dave@pawfal.org>
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

#include <config.h>
#include <cstring>
#include <dlfcn.h>
#include <stdio.h>
#include <iostream>
#include "DeviceClassRegistry.h"
#include "PanelRegistry.h"
#include "Gzip.h"
#include "PluginManifest.h"
#include "PassthroughPlugin.h"
#include <algorithm>
#include <dirent.h>
#include <unistd.h>

using namespace std;
using namespace Spiral::Core;

DeviceClassRegistry *DeviceClassRegistry::m_Singleton = NULL;

DeviceClassRegistry::DeviceClassRegistry()
{
}

DeviceClassRegistry::~DeviceClassRegistry()
{
	for (vector<DeviceClass*>::iterator i=m_PluginVec.begin();
	     i!=m_PluginVec.end(); i++)
	{
		delete *i;
	}
	m_PluginVec.clear();
}

DeviceClassRegistry *DeviceClassRegistry::Get()
{
	if (!m_Singleton)
	{
		m_Singleton = new DeviceClassRegistry;
		PluginManager::Get()->RegisterKind(m_Singleton);
	}
	return m_Singleton;
}

void DeviceClassRegistry::PackUpAndGoHome()
{
	if (!m_Singleton) return;
	PluginManager::Get()->UnregisterKind(m_Singleton);
	delete m_Singleton;
	m_Singleton = NULL;
}

SpiralPlugin *DeviceClass::CreateInstance() const
{
	SpiralPlugin *plugin = Build ? Build(*this) : (Create ? Create() : NULL);

	if (plugin)
		plugin->Named(Name);

	return plugin;
}

BuiltinDevice::BuiltinDevice(ClassFunction function)
{
	List().push_back(function);
}

vector<BuiltinDevice::ClassFunction> &BuiltinDevice::List()
{
	static vector<ClassFunction> list;

	return list;
}

const vector<BuiltinDevice::ClassFunction> &BuiltinDevice::All()
{
	return List();
}

unsigned DeviceClassRegistry::RegisterBuiltins()
{
	unsigned count = 0;
	const vector<BuiltinDevice::ClassFunction> &builtins = BuiltinDevice::All();

	for (size_t i = 0; i < builtins.size(); ++i)
		if (Register(builtins[i]()))
			++count;

	return count;
}

bool DeviceClassRegistry::ReadManifest(const unsigned char *data, size_t size, DeviceClass &device, string &error)
{
	string text;

	if (!data || !Spumoni::Gzip::Decompress(data, size, text, error))
		return false;

	PluginManifest manifest;

	if (!manifest.Parse(text, error))
		return false;

	if (manifest.type != "dsp" && manifest.type != "audio-driver")
	{
		error = "not a dsp manifest";

		return false;
	}

	device.ID = manifest.id;
	device.Name = manifest.name;
	device.Category = manifest.category;
	device.Panel = manifest.ui;

	return true;
}

unsigned DeviceClassRegistry::LoadModules(const string &root)
{
	m_Root = root;
	unsigned count = RegisterBuiltins();
	count += LoadBuiltins(root);

	return count + PluginManager::Get()->Load(*this, root);
}

unsigned DeviceClassRegistry::LoadBuiltins(const string &root)
{
	string base = root;
	if (!base.empty() && base[base.size() - 1] != '/')
		base += '/';
	base += Subdirectory();
	DIR *dir = opendir(base.c_str());
	if (!dir)
		return 0;
	vector<string> names;
	while (struct dirent *entry = readdir(dir))
		if (entry->d_name[0] != '.')
			names.push_back(entry->d_name);
	closedir(dir);
	sort(names.begin(), names.end());

	unsigned count = 0;
	for (size_t i = 0; i < names.size(); ++i)
	{
		const string manifest = base + "/" + names[i] + "/info.json";
		if (access(manifest.c_str(), R_OK) != 0)
			continue;
		DeviceClass device;
		if (PassthroughPlugin::ReadClass(manifest, device) && Register(device))
			++count;
	}
	return count;
}

bool DeviceClassRegistry::Accept(void *entry, const string &path)
{
	void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_NOLOAD);
	if (handle == NULL)
	{
		cerr << "Error loading [" << path << "]: " << dlerror() << endl;
		return false;
	}

	typedef const char *(*TextFn)();
	TextFn GetHostABI = (TextFn)dlsym(handle, "SpiralPlugin_GetHostABI");
	const char *abi = GetHostABI ? GetHostABI() : NULL;

	int (*GetType)(void) = (int(*)()) dlsym(handle, "SpiralPlugin_GetType");
	const char **(*GetIcon)(void) = (const char **(*)()) dlsym(handle, "SpiralPlugin_GetIcon");
	typedef const unsigned char *(*ManifestFn)(size_t *);
	ManifestFn GetManifest = (ManifestFn)dlsym(handle, "SpiralPlugin_GetManifest");
	dlclose(handle);

	if (!abi || strcmp(abi, SSM_HOST_ABI) != 0)
	{
		cerr << "Incompatible device module " << path << " (module ABI "
		     << (abi ? abi : "unspecified") << ", host ABI " << SSM_HOST_ABI
		     << "). Rebuild the module for this host." << endl;
		return false;
	}

	if (!GetType || GetType() != SPIRAL_PLUGIN_TYPE_DEVICE)
	{
		cerr << "Obsolete or invalid plugin module: " << path << endl;
		return false;
	}

	if (!GetIcon || !GetManifest)
	{
		cerr << "Error linking to plugin " << path << endl;
		return false;
	}

	DeviceClass device;
	size_t size = 0;
	const unsigned char *manifest = GetManifest(&size);
	string error;

	if (!ReadManifest(manifest, size, device, error))
	{
		cerr << "Invalid embedded manifest in " << path << ": " << error << endl;
		return false;
	}

	device.Icon = GetIcon();
	device.Create = (SpiralPlugin *(*)()) entry;
	device.Module = path;

	if (device.ID < 0)
		return false;

	if (device.Name.empty())
	{
		cerr << "Missing plugin name: " << path << endl;
		return false;
	}

	return Register(device);
}

void DeviceClassRegistry::Release(void *, const string &path)
{
	for (vector<DeviceClass*>::iterator i=m_PluginVec.begin();
	     i!=m_PluginVec.end();)
	{
		if ((*i)->Module == path)
		{
			PanelRegistry::Get()->Forget((*i)->ID);
			delete *i;
			i = m_PluginVec.erase(i);
		}
		else i++;
	}
}

bool DeviceClassRegistry::Register(const DeviceClass &device)
{
	if (device.ID < 0 || Find(device.ID)) return false;
	m_PluginVec.push_back(new DeviceClass(device));
	if (!device.Panel.empty() || device.PanelGenerated || (!m_Root.empty() && PanelRegistry::HasPanelFile(device.ID)))
		PanelRegistry::Get()->Offer(device);
	return true;
}

const DeviceClass *DeviceClassRegistry::Find(int ID) const
{
	for (vector<DeviceClass*>::const_iterator i=m_PluginVec.begin();
	     i!=m_PluginVec.end(); i++)
	{
		if ((*i)->ID==ID)
			return *i;
	}
	return NULL;
}
