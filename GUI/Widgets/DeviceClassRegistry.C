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

using namespace std;

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
		spiralcore::PluginLoader::Get()->RegisterKind(m_Singleton);
	}
	return m_Singleton;
}

void DeviceClassRegistry::PackUpAndGoHome()
{
	if (!m_Singleton) return;
	// Releases every module of this kind first.
	spiralcore::PluginLoader::Get()->UnregisterKind(m_Singleton);
	delete m_Singleton;
	m_Singleton = NULL;
}

unsigned DeviceClassRegistry::LoadModules(const string &root)
{
	return spiralcore::PluginLoader::Get()->Load(*this, root);
}

bool DeviceClassRegistry::Accept(void *entry, const string &path)
{
	// The loader holds the module; this is its handle counted once more
	// while the old exports beside the entry are read.
	void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_NOLOAD);
	if (handle == NULL)
	{
		cerr << "Error loading [" << path << "]: " << dlerror() << endl;
		return false;
	}

	typedef const char *(*TextFn)();
	TextFn GetHostABI = (TextFn)dlsym(handle, "SpiralPlugin_GetHostABI");
	const char *abi = GetHostABI ? GetHostABI() : NULL;

	int (*GetID)(void) = (int(*)()) dlsym(handle, "SpiralPlugin_GetID");
	int (*GetType)(void) = (int(*)()) dlsym(handle, "SpiralPlugin_GetType");
	const char **(*GetIcon)(void) = (const char **(*)()) dlsym(handle, "SpiralPlugin_GetIcon");
	std::string (*GetGroupName)(void) =
		(std::string(*)()) dlsym(handle, "SpiralPlugin_GetGroupName");
	std::string (*GetName)(void) = (std::string(*)()) dlsym(handle, "SpiralPlugin_GetName");
	dlclose(handle);

	if (!abi || strcmp(abi, SSM_HOST_ABI) != 0)
	{
		cerr << "Missing or incompatible plugin ABI: " << path << endl;
		return false;
	}

	if (!GetID || !GetType || GetType() != SPIRAL_PLUGIN_TYPE_DSP)
	{
		cerr << "Obsolete or invalid plugin module: " << path << endl;
		return false;
	}

	if (!GetIcon || !GetGroupName || !GetName)
	{
		cerr << "Error linking to plugin " << path << endl;
		return false;
	}

	DeviceClass device;
	device.ID = GetID();
	device.Name = GetName();
	device.Category = GetGroupName();
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

	// A second module with this ID is not an error; it is just not kept.
	return Register(device);
}

void DeviceClassRegistry::Release(void *, const string &path)
{
	for (vector<DeviceClass*>::iterator i=m_PluginVec.begin();
	     i!=m_PluginVec.end();)
	{
		if ((*i)->Module == path)
		{
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
