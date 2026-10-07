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
#include "EditorClassRegistry.h"

using namespace std;
using namespace spiralcore;

EditorClassRegistry *EditorClassRegistry::m_Singleton = NULL;

EditorClassRegistry::EditorClassRegistry()
{
}

EditorClassRegistry::~EditorClassRegistry()
{
	for (vector<EditorClass*>::iterator i=m_PluginVec.begin();
	     i!=m_PluginVec.end(); i++)
	{
		delete *i;
	}
	m_PluginVec.clear();
}

EditorClassRegistry *EditorClassRegistry::Get()
{
	if (!m_Singleton)
	{
		m_Singleton = new EditorClassRegistry;
		PluginManager::Get()->RegisterKind(m_Singleton);
	}
	return m_Singleton;
}

void EditorClassRegistry::PackUpAndGoHome()
{
	if (!m_Singleton) return;
	// Releases every module of this kind first.
	PluginManager::Get()->UnregisterKind(m_Singleton);
	delete m_Singleton;
	m_Singleton = NULL;
}

unsigned EditorClassRegistry::LoadModules(const string &root)
{
	return PluginManager::Get()->Load(*this, root);
}

bool EditorClassRegistry::Accept(void *entry, const string &path)
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
	dlclose(handle);

	if (!abi || strcmp(abi, SSM_HOST_ABI) != 0)
	{
		cerr << "Missing or incompatible plugin ABI: " << path << endl;
		return false;
	}

	if (!GetID || !GetType || GetType() != SPIRAL_PLUGIN_TYPE_EDITOR)
	{
		cerr << "Obsolete or invalid plugin module: " << path << endl;
		return false;
	}

	EditorClass editor;
	editor.ForDevice = GetID();
	editor.Toolkit = "fltk";
	editor.Icon = GetIcon ? GetIcon() : NULL;
	editor.Create = (SpiralGUIType *(*)(SpiralPlugin *)) entry;
	editor.Module = path;

	if (editor.ForDevice < 0)
		return false;

	// A second editor for this device is not an error; it is just not kept.
	return Register(editor);
}

void EditorClassRegistry::Release(void *, const string &path)
{
	for (vector<EditorClass*>::iterator i=m_PluginVec.begin();
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

bool EditorClassRegistry::Register(const EditorClass &editor)
{
	if (editor.ForDevice < 0 || Find(editor.ForDevice, editor.Toolkit)) return false;
	m_PluginVec.push_back(new EditorClass(editor));
	return true;
}

const EditorClass *EditorClassRegistry::Find(int ForDevice, const string &Toolkit) const
{
	for (vector<EditorClass*>::const_iterator i=m_PluginVec.begin();
	     i!=m_PluginVec.end(); i++)
	{
		if ((*i)->ForDevice==ForDevice && (*i)->Toolkit==Toolkit)
			return *i;
	}
	return NULL;
}
