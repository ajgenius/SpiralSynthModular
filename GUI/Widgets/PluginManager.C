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

#include <dlfcn.h>
#include <stdio.h>
#include "SpiralInfo.h"
#include "PluginManager.h"
#include "SpiralGUI.H"

using namespace std;

PluginManager *PluginManager::m_Singleton = NULL;

PluginManager::PluginManager()
{
}

PluginManager::~PluginManager()
{
	UnloadAll();
}

static void ClearDSP(HostsideInfo *p)
{
	p->dsp.Handle = NULL;
	p->dsp.CreateInstance = NULL;
	p->dsp.GetIcon = NULL;
	p->dsp.GetGroupName = NULL;
}

static void ClearGUI(HostsideInfo *p)
{
	p->gui.Handle = NULL;
	p->gui.CreateGUI = NULL;
	p->gui.GetIcon = NULL;
}

static void UpdatePairedType(HostsideInfo *p)
{
	if (p->dsp.Handle && p->gui.Handle)
		p->type = SPIRAL_PLUGIN_TYPE_PAIRED;
	else if (p->dsp.Handle)
		p->type = SPIRAL_PLUGIN_TYPE_DSP;
	else if (p->gui.Handle)
		p->type = SPIRAL_PLUGIN_TYPE_GUI;
}

HostsideInfo *PluginManager::NewSlot(int ID)
{
	HostsideInfo *p = new HostsideInfo;
	p->ID = ID;
	p->type = -1;
	ClearDSP(p);
	ClearGUI(p);
	m_PluginVec.push_back(p);
	return p;
}

PluginID PluginManager::LoadPlugin(const char *PluginName)
{
	// DSP modules load first so GUI methods resolve immediately.
	void *handle = dlopen(PluginName, RTLD_NOW | RTLD_GLOBAL);
	if (handle == NULL)
	{
		SpiralInfo::Alert("Error loading ["+string(PluginName)+"]: \n"+string(dlerror()));
		return PluginError;
	}

	char *error = NULL;
	dlerror();

	int (*GetID)(void) = (int(*)()) dlsym(handle, "SpiralPlugin_GetID");
	if ((error = dlerror()) != NULL)
	{
		SpiralInfo::Alert("Error linking to plugin "+string(PluginName)+"\n"+string(error));
		dlclose(handle);
		return PluginError;
	}
	int ID = GetID();
	if (ID < 0)
	{
		dlclose(handle);
		return PluginError;
	}

	int type = 0;
	int (*GetType)(void) = (int(*)()) dlsym(handle, "SpiralPlugin_GetType");
	if (dlerror() == NULL && GetType)
		type = GetType();

	if (type != SPIRAL_PLUGIN_TYPE_DSP && type != SPIRAL_PLUGIN_TYPE_GUI)
	{
		SpiralInfo::Alert("Obsolete or invalid plugin module: "+string(PluginName));
		dlclose(handle);
		return PluginError;
	}

	HostsideInfo *slot = GetPlugin_i(ID);
	if (!slot)
		slot = NewSlot(ID);

	if (type == SPIRAL_PLUGIN_TYPE_GUI)
	{
		if (slot->gui.Handle)
		{
			dlclose(handle);
			return ID;
		}

		SpiralGUIType *(*CreateGUI)(SpiralPlugin *) =
			(SpiralGUIType *(*)(SpiralPlugin *)) dlsym(handle, "SpiralPlugin_CreateGUI");
		if ((error = dlerror()) != NULL)
		{
			SpiralInfo::Alert("Error linking GUI in "+string(PluginName)+"\n"+string(error));
			dlclose(handle);
			return PluginError;
		}

		const char **(*GetIcon)(void) = (const char **(*)()) dlsym(handle, "SpiralPlugin_GetIcon");
		if (dlerror() != NULL)
			GetIcon = NULL;

		slot->gui.Handle = handle;
		slot->gui.CreateGUI = CreateGUI;
		slot->gui.GetIcon = GetIcon;
		UpdatePairedType(slot);
		return ID;
	}

	// Resolve the DSP factory and its metadata.
	if (slot->dsp.Handle)
	{
		dlclose(handle);
		return ID;
	}

	SpiralPlugin *(*CreateInstance)(void) =
		(SpiralPlugin *(*)()) dlsym(handle, "SpiralPlugin_CreateInstance");
	if ((error = dlerror()) != NULL)
	{
		SpiralInfo::Alert("Error linking to plugin "+string(PluginName)+"\n"+string(error));
		dlclose(handle);
		return PluginError;
	}

	const char **(*GetIcon)(void) = (const char **(*)()) dlsym(handle, "SpiralPlugin_GetIcon");
	if ((error = dlerror()) != NULL)
	{
		SpiralInfo::Alert("Error linking to plugin "+string(PluginName)+"\n"+string(error));
		dlclose(handle);
		return PluginError;
	}

	std::string (*GetGroupName)(void) =
		(std::string(*)()) dlsym(handle, "SpiralPlugin_GetGroupName");
	if ((error = dlerror()) != NULL)
	{
		SpiralInfo::Alert("Error linking to plugin "+string(PluginName)+"\n"+string(error));
		dlclose(handle);
		return PluginError;
	}

	slot->dsp.Handle = handle;
	slot->dsp.CreateInstance = CreateInstance;
	slot->dsp.GetIcon = GetIcon;
	slot->dsp.GetGroupName = GetGroupName;
	UpdatePairedType(slot);
	return ID;
}

void PluginManager::UnLoadPlugin(PluginID ID)
{
	HostsideInfo *p = GetPlugin_i(ID);
	if (!p) return;
	if (p->gui.Handle) { dlclose(p->gui.Handle); ClearGUI(p); }
	if (p->dsp.Handle) { dlclose(p->dsp.Handle); ClearDSP(p); }
	p->type = 0;
	char *error;
	if ((error = dlerror()) != NULL)
		SpiralInfo::Alert("Error unlinking plugin: \n"+string(error));
}

void PluginManager::UnloadAll()
{
	for (vector<HostsideInfo*>::iterator i=m_PluginVec.begin();
	     i!=m_PluginVec.end(); i++)
	{
		if ((*i)->gui.Handle) dlclose((*i)->gui.Handle);
		if ((*i)->dsp.Handle) dlclose((*i)->dsp.Handle);
		delete *i;
	}
	m_PluginVec.clear();
}

const HostsideInfo *PluginManager::GetPlugin(PluginID ID)
{
	HostsideInfo *ret = GetPlugin_i(ID);
	if (!ret)
	{
		char t[256];
		sprintf(t,"%d",ID);
		SpiralInfo::Alert("Plugin "+string(t)+" not found.");
	}
	return ret;
}

HostsideInfo *PluginManager::GetPlugin_i(PluginID ID)
{
	for (vector<HostsideInfo*>::iterator i=m_PluginVec.begin();
	     i!=m_PluginVec.end(); i++)
	{
		if ((*i)->ID==ID)
			return *i;
	}
	return NULL;
}

bool PluginManager::IsValid(PluginID ID)
{
	const HostsideInfo *t = GetPlugin(ID);
	return (t && t->HasDSP());
}

int PluginManager::GetIdByName(string Name)
{
	/* Declared on UA PluginManager.h but never defined. Looking up by
	   PluginInfo.Name would mean constructing a DSP instance at scan
	   time; the FLTK host never calls this. */
	(void)Name;
	return PluginError;
}

