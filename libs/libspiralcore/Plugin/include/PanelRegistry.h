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

#ifndef SPIRAL_PANEL_REGISTRY_H
#define SPIRAL_PANEL_REGISTRY_H

#include <string>
#include <vector>
#include "DeviceClassRegistry.h"

class SpiralGUIType;

namespace Spiral
{
namespace Core
{

struct PanelClass
{
	int   ForDevice;
	std::string Toolkit;
	const char **Icon;
	SpiralGUIType *(*CreatePanel)(SpiralPlugin *, const DeviceClass &);

	PanelClass(): ForDevice(-1), Icon(NULL), CreatePanel(NULL) {}

	SpiralGUIType *Open(SpiralPlugin *plugin) const
	{
		const DeviceClass *device = plugin && CreatePanel ? DeviceClassRegistry::Get()->Find(ForDevice) : NULL;
		return device ? CreatePanel(plugin, *device) : NULL;
	}
};

class PanelRegistry
{
public:
	static PanelRegistry *Get();
	static void         PackUpAndGoHome();

	bool                Register(const PanelClass &panel);
	void                ProvidePanels(const std::string &toolkit, SpiralGUIType *(*create)(SpiralPlugin *, const DeviceClass &));
	void                Offer(const DeviceClass &device);
	static std::string  PanelFolder();
	void                Forget(int ForDevice);
	const PanelClass*  Find(int ForDevice, const std::string &Toolkit = "fltk") const;
	const std::vector<PanelClass*> &Classes() const { return m_PluginVec; }

private:

	PanelRegistry();
	~PanelRegistry();

	struct PanelProvider
	{
		std::string Toolkit;
		SpiralGUIType *(*Create)(SpiralPlugin *, const DeviceClass &);
	};

	std::vector<PanelClass*> m_PluginVec;
	std::vector<PanelProvider> m_Providers;
	static PanelRegistry *m_Singleton;
};

}
}

#endif
