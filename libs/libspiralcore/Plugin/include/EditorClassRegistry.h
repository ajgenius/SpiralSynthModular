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

#ifndef SPIRAL_EDITOR_CLASS_REGISTRY_H
#define SPIRAL_EDITOR_CLASS_REGISTRY_H

#include <string>
#include <vector>
#include "DeviceClassRegistry.h"

class SpiralGUIType;

namespace spiralcore
{

/* The GUI half of the paired slot (0.3.1 split): an editor of the device
   class it names by ID. It sees the device only as a SpiralPlugin and its
   ChannelHandler, so the device need not be the native DSP module of the
   same name, and several editors (toolkits, variants) may name one
   device. The modules under panels/ are FLTK editors. */
struct EditorClass
{
	int   ForDevice;
	std::string Toolkit;
	const char **Icon;
	SpiralGUIType *(*Create)(SpiralPlugin *);
	std::string Module;

	SpiralGUIType *CreateEditor(SpiralPlugin *plugin) const
	{
		return (Create && plugin) ? Create(plugin) : NULL;
	}
};

//////////////////////////////////////////////////////////

/* The editor kind: modules are <plugins>/panels/<X>/<X>_GUI<ext>, the entry
   is the editor factory, and the device ID beside it is the old export
   set, read back through the module until modules carry a descriptor. */
class EditorClassRegistry : public PluginKind
{
public:
	static EditorClassRegistry *Get();
	static void         PackUpAndGoHome();

	virtual const char *Name() const { return "editor"; }
	virtual const char *Subdirectory() const { return "panels"; }
	virtual const char *Suffix() const { return "_GUI"; }
	virtual const char *EntrySymbol() const { return "SpiralPlugin_CreateGUI"; }
	virtual bool        Accept(void *entry, const std::string &path);
	virtual void        Release(void *entry, const std::string &path);

	unsigned            LoadModules(const std::string &root);
	// The first editor registered for a device and toolkit is the one found.
	bool                Register(const EditorClass &editor);
	const EditorClass*  Find(int ForDevice, const std::string &Toolkit = "fltk") const;
	const std::vector<EditorClass*> &Classes() const { return m_PluginVec; }

private:

	EditorClassRegistry();
	~EditorClassRegistry();

	std::vector<EditorClass*> m_PluginVec;
	static EditorClassRegistry *m_Singleton;
};

}

#endif
