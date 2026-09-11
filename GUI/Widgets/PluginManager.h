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

#ifndef SPIRAL_PLUGIN_MANAGER_H
#define SPIRAL_PLUGIN_MANAGER_H

#include <string>
#include <vector>
#include "SpiralPlugin.h"

class SpiralGUIType;

/* Paired DSP+GUI plugin (0.3.1 split). type is SPIRAL_PLUGIN_TYPE_DSP/GUI
   while only one side is loaded, SPIRAL_PLUGIN_TYPE_PAIRED once both .so
   files for the same ID are in. GUI<->DSP traffic stays on ChannelHandler
   (libspiralcore mutex trylock); this is not master's Device/Property
   split and not the lock-free queues from PR #11. */
struct HostsideInfo
{
	int   ID;
	int   type;

	struct {
		void *Handle;
		SpiralPlugin *(*CreateInstance)(void);
		const char **(*GetIcon)(void);
		std::string (*GetGroupName)(void);
	} dsp;

	struct {
		void *Handle;
		SpiralGUIType *(*CreateGUI)(SpiralPlugin *);
		const char **(*GetIcon)(void);
	} gui;

	SpiralPlugin *CreateDSPInstance() const
	{
		return (dsp.CreateInstance) ? dsp.CreateInstance() : NULL;
	}

	SpiralGUIType *CreateGUI(SpiralPlugin *plugin) const
	{
		return (gui.CreateGUI && plugin) ? gui.CreateGUI(plugin) : NULL;
	}

	const char **Icon() const
	{
		if (dsp.GetIcon) return dsp.GetIcon();
		if (gui.GetIcon) return gui.GetIcon();
		return NULL;
	}

	std::string GroupName() const
	{
		return dsp.GetGroupName ? dsp.GetGroupName() : std::string();
	}

	bool HasDSP() const { return dsp.Handle != NULL && dsp.CreateInstance != NULL; }
};

//////////////////////////////////////////////////////////

typedef int PluginID;
#define     PluginError -1

class PluginManager
{
public:
	static PluginManager *Get() { if(!m_Singleton) m_Singleton=new PluginManager; return m_Singleton; }
	static void         PackUpAndGoHome() { if(m_Singleton) delete m_Singleton; }

	PluginID            LoadPlugin(const char *PluginName);
	void                UnLoadPlugin(PluginID ID);
	void                UnloadAll();
	const HostsideInfo* GetPlugin(PluginID ID);
	bool                IsValid(PluginID ID);
	int                 GetIdByName(std::string Name);

private:

	PluginManager();
	~PluginManager();
	HostsideInfo *GetPlugin_i(PluginID ID);
	HostsideInfo *NewSlot(int ID);

	std::vector<HostsideInfo*> m_PluginVec;
	static PluginManager *m_Singleton;
};

#endif
