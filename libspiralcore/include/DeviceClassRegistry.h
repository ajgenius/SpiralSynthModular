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

#ifndef SPIRAL_DEVICE_CLASS_REGISTRY_H
#define SPIRAL_DEVICE_CLASS_REGISTRY_H

#include <string>
#include <vector>
#include "PluginManager.h"

class SpiralPlugin;

namespace spiralcore
{

// What a module's SpiralPlugin_GetType export answers.
enum
{
	SPIRAL_PLUGIN_TYPE_DSP = 1,
	SPIRAL_PLUGIN_TYPE_GUI = 2,
	SPIRAL_PLUGIN_TYPE_PAIRED = 3
};

/* The DSP half of the paired slot (0.3.1 split), one per plugin ID. A
   module's is read from its exports when the loader accepts it; a
   compiled-in device registers one with no module. An editor pairs with
   it by ID through the EditorClassRegistry. GUI<->DSP traffic stays on
   ChannelHandler (libspiralcore mutex trylock). */
struct DeviceClass
{
	int   ID;
	std::string Name;
	std::string Category;
	const char **Icon;
	SpiralPlugin *(*Create)(void);
	std::string Module;

	SpiralPlugin *CreateInstance() const
	{
		return (Create) ? Create() : NULL;
	}
};

//////////////////////////////////////////////////////////

/* The device kind: modules are <plugins>/dsp/<X>/<X>_DSP<ext>, the entry
   is the instance factory, and the identity beside it (ID, type, name,
   category, icon, host ABI) is the old export set, read back through the
   module until modules carry a descriptor. */
class DeviceClassRegistry : public PluginKind
{
public:
	static DeviceClassRegistry *Get();
	static void         PackUpAndGoHome();

	virtual const char *Name() const { return "device"; }
	virtual const char *Subdirectory() const { return "dsp"; }
	virtual const char *Suffix() const { return "_DSP"; }
	virtual const char *EntrySymbol() const { return "SpiralPlugin_CreateInstance"; }
	virtual bool        Accept(void *entry, const std::string &path);
	virtual void        Release(void *entry, const std::string &path);

	unsigned            LoadModules(const std::string &root);
	// The first registration of an ID keeps its place.
	bool                Register(const DeviceClass &device);
	const DeviceClass*  Find(int ID) const;
	const std::vector<DeviceClass*> &Classes() const { return m_PluginVec; }

private:

	DeviceClassRegistry();
	~DeviceClassRegistry();

	std::vector<DeviceClass*> m_PluginVec;
	static DeviceClassRegistry *m_Singleton;
};

}

#endif
