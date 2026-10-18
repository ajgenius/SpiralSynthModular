// Copyright (C) 2007 ajgenius <113943+ajgenius@users.noreply.github.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include "Device.h"

#include <cstdio>
#include <iostream>
#include <typeinfo>

//#define DEBUG_LOADING
using namespace Spiral::Core;

Device::Device()
{
	/* Used For Loading Information, and User Graph */
	DeviceVersion = UnsignedProperty(Property::ReadOnly, 1, 0, 0, 0, 0);
	DeviceName = StringProperty(Property::ReadOnly, "Device");
}

Device::~Device()
{
}

bool Device::LoadProperties(std::map<std::string, Property *> &properties)
{
	/* See if this is a versioned set of properties */
	if (properties.count("VERSION"))
	{
		UnsignedProperty *version = dynamic_cast<UnsignedProperty*>(properties["VERSION"]);

		if (version->Value.AsUnsigned != DeviceVersion.Value.AsUnsigned)
		{
			std::cerr << "Version Mismatch (" << version->Value.AsUnsigned << " != " << DeviceVersion.Value.AsUnsigned 
				<<") in property loading! This probably indicates a bug in " << DeviceName.Value << std::endl;

			return false;
		}
	}
	
	std::map<std::string, Property *>::iterator iter;
	for (iter = m_State.Properties.begin(); iter != m_State.Properties.end(); iter++)
	{
		if (iter->second && iter->second->IsReadOnly())
			continue;


		#ifdef DEBUG_LOADING
		printf("Maybe Loading Property - %s\n", iter->first.c_str());
		#endif

		if (properties.count(iter->first))
		{
			#ifdef DEBUG_LOADING
			printf("Almost Certainly Loading Property - %s which is of type %s\n", iter->first.c_str(), iter->second->ClassName().c_str());
			#endif

			{
				iter->second->CopyValue(*properties[iter->first]);

				if (iter->second->RequiresUpdates())
				{
					UpdateProperty(iter->first);
				}
			}
		}
	}

	return true;
}

bool Device::CopyDevice(Device *device)
{
	if (!device)
	{
		// Gimme an error
		return false;
	}

	if (typeid(device) != typeid(this))
	{
		// Gimme an error
		return false;
	}

	return LoadProperties(device->m_State.Properties);
}
