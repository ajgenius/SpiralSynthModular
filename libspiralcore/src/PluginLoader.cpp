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
#include <dirent.h>
#include <sys/stat.h>
#include <iostream>
#include <algorithm>
#include "PluginLoader.h"

using namespace std;
using namespace spiralcore;

PluginLoader *PluginLoader::m_Singleton = NULL;

PluginLoader::PluginLoader()
{
}

PluginLoader::~PluginLoader()
{
	UnloadAll();
}

void PluginLoader::PackUpAndGoHome()
{
	if (m_Singleton && !m_Singleton->UnloadAll())
		return;

	delete m_Singleton;
	m_Singleton = NULL;
}

bool PluginLoader::UnregisterKind(PluginKind *kind)
{
	if (!kind || !kind->CanUnload())
		return false;

	for (size_t n = m_Modules.size(); n > 0; --n)
	{
		if (m_Modules[n - 1].Kind != kind)
			continue;

		ReleaseModule(m_Modules[n - 1]);
		m_Modules.erase(m_Modules.begin() + n - 1);
	}

	m_Kinds.erase(remove(m_Kinds.begin(), m_Kinds.end(), kind), m_Kinds.end());
	return true;
}

void PluginLoader::RegisterKind(PluginKind *kind)
{
	if (!kind) return;
	for (vector<PluginKind*>::iterator i=m_Kinds.begin(); i!=m_Kinds.end(); i++)
		if (*i == kind) return;

	m_Kinds.push_back(kind);
}

static bool HasSuffix(const string &name, const char *suffix)
{
	const string ending = string(suffix) + PLUGIN_MODULE_EXT;
	return name.size() > ending.size() && name.compare(name.size() - ending.size(), ending.size(), ending) == 0;
}

unsigned PluginLoader::Load(PluginKind &kind, const string &root)
{
	string directory = root;
	if (!directory.empty() && directory[directory.size()-1] != '/') directory += '/';
	directory += kind.Subdirectory();

	unsigned loaded = 0;
	DIR *dir = opendir(directory.c_str());
	if (!dir) return 0;

	while (struct dirent *entry = readdir(dir))
	{
		const string name = entry->d_name;
		if (name == "." || name == "..") continue;

		const string path = directory + "/" + name;
		struct stat st;
		if (stat(path.c_str(), &st)) continue;

		if (S_ISDIR(st.st_mode))
		{
			DIR *sub = opendir(path.c_str());
			if (!sub) continue;

			while (struct dirent *module = readdir(sub))
				if (HasSuffix(module->d_name, kind.Suffix()) && LoadModule(kind, path + "/" + module->d_name))
					++loaded;

			closedir(sub);
		}
		else if (HasSuffix(name, kind.Suffix()) && LoadModule(kind, path))
			++loaded;
	}
	closedir(dir);
	return loaded;
}

unsigned PluginLoader::LoadAll(const string &root)
{
	unsigned loaded = 0;
	for (vector<PluginKind*>::iterator i=m_Kinds.begin(); i!=m_Kinds.end(); i++)
		loaded += Load(**i, root);

	return loaded;
}

bool PluginLoader::LoadModule(PluginKind &kind, const string &path)
{
	void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
	if (handle == NULL)
	{
		cerr << kind.Name() << " plugin " << path << ": " << dlerror() << endl;
		return false;
	}

	void *entry = dlsym(handle, kind.EntrySymbol());
	if (!entry)
	{
		cerr << kind.Name() << " plugin " << path << ": no " << kind.EntrySymbol() << endl;
		dlclose(handle);
		return false;
	}

	// The kind decides; it says why on its own.
	if (!kind.Accept(entry, path))
	{
		dlclose(handle);
		return false;
	}

	Module module = { &kind, handle, entry, path };
	m_Modules.push_back(module);
	return true;
}

void PluginLoader::ReleaseModule(const Module &module)
{
	module.Kind->Release(module.Entry, module.Path);
	dlclose(module.Handle);
}

bool PluginLoader::UnloadAll()
{
	// Check the whole set before releasing any descriptor or library.
	for (size_t n = 0; n < m_Modules.size(); ++n)
		if (!m_Modules[n].Kind->CanUnload())
			return false;

	for (size_t n = m_Modules.size(); n > 0; --n)
		ReleaseModule(m_Modules[n - 1]);

	m_Modules.clear();
	return true;
}
