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

namespace Spiral
{
namespace Core
{

enum
{
	SPIRAL_PLUGIN_TYPE_DEVICE = 1,
	SPIRAL_PLUGIN_TYPE_EDITOR = 2,
	SPIRAL_PLUGIN_TYPE_PAIRED = 3
};

struct DeviceClass
{
	int   ID;
	std::string Name;
	std::string Category;
	const char **Icon;
	SpiralPlugin *(*Create)(void);
	std::string Module;
	SpiralPlugin *(*Build)(const DeviceClass &device);
	std::string Manifest;
	bool PanelGenerated;

	DeviceClass() : ID(-1), Icon(NULL), Create(NULL), Build(NULL), PanelGenerated(false) {}

	SpiralPlugin *CreateInstance() const
	{
		if (Build)
			return Build(*this);
		return (Create) ? Create() : NULL;
	}

	std::string Directory() const
	{
		const std::string &file = Module.empty() ? Manifest : Module;
		const std::string::size_type slash = file.rfind('/');
		return (slash == std::string::npos) ? std::string() : file.substr(0, slash);
	}
};

//////////////////////////////////////////////////////////

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
	unsigned            LoadBuiltins(const std::string &root);
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
}

#endif
