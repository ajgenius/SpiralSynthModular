/*  SpiralSound
 *  Copyleft (C) 2001 David Griffiths <dave@pawfal.org>
 *  Passthrough device from the Scope plugin: GROK (Grok Bot), 2026
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

#ifndef PASSTHROUGHPLUGIN
#define PASSTHROUGHPLUGIN

#include "Device.h"
#include "PluginManifest.h"
#include "DeviceClassRegistry.h"
#include <string>
#include <vector>

class PassthroughPlugin : public Spiral::Core::Device
{
public:
	explicit PassthroughPlugin(const PluginManifest &manifest);
	virtual ~PassthroughPlugin();
	virtual void Execute();
	virtual void Describe(Spiral::Core::Description &d);
	virtual void Apply(Spiral::Core::Description::Reader &r);

	static bool ReadClass(const std::string &manifest, Spiral::Core::DeviceClass &device);

protected:
	struct Held
	{
		PluginManifest::Property Manifest;
		Spiral::Core::Property *Property;
	};

	static SpiralPlugin *Build(const Spiral::Core::DeviceClass &device);
	static const char **Icon(const std::string &directory);

	int m_Pairs;
	int m_StateVersion;
	std::vector<Held> m_Held;
};

#endif
