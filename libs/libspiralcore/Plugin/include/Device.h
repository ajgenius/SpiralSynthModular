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

#ifndef SPIRALCORE_DEVICE_H
#define SPIRALCORE_DEVICE_H

#include <algorithm>
#include <cassert>
#include <cctype>
#include <map>
#include <string>
#include <vector>

#include "ChannelHandler.h"
#include "Property.h"
#include "SpiralPlugin.h"

namespace Spiral
{
namespace Core
{
	class Device : public SpiralPlugin
	{
	public:
		Device();
		virtual ~Device();

		/* Setting Properties */
		virtual bool LoadProperties(std::map<std::string, Property *> &properties);
		bool CopyDevice(Device *device);
		virtual bool UpdateProperty(std::string name) { return true; }

		virtual void Describe(Description &d);
		virtual void Apply(Description::Reader &r);

		/* Shared Properties */
		inline Property* DuplicateProperty(std::string &Key)
		{
			if (m_State.Properties.count(Key))
			{
				return m_State.Properties[Key]->Duplicate();
			}

			return NULL;
		}

		inline void DuplicateProperties(std::map<std::string, Property *> &External)
		{
			std::map<std::string, Property *>::iterator prop;

			for (prop = m_State.Properties.begin();
				prop != m_State.Properties.end();
				prop++)
			{
				if (!External.count(prop->first))
					External[prop->first] = prop->second->Duplicate();
			}
		}

		inline void SynchronizeProperty(std::string Key, Property *Value)
		{
			if (m_State.Properties.count(Key))
			{
				Property *shared = m_State.Properties[Key];

				if (shared->IsReadOnly())
					Value->CopyValue(*shared);
				else
				{
					shared->CopyValue(*Value);

					if (shared->RequiresUpdates())
						UpdateProperty(Key);
				}

				return;
			}

			//WARNING?
		}

	protected:
		/* Per Class Properties*/
		UnsignedProperty DeviceVersion;
		StringProperty DeviceName;

		/* State Properties */
		struct
		{
			/* Per Instance Properties */
			std::map<std::string, Property *> Properties;
			std::map<Property *, Property *> UpdateData; /* For ProcessUpdate */
			std::vector<Property *> Order;
		} m_State;


		#define ValueAsInt(property) property.Value.AsSigned
		#define ValueAsUInt(property) property.Value.AsUnsigned
		#define ValueAsFloat(property) property.Value.AsFloat
		#define ValueSetInt(property, new) property.Value.AsSigned = new
		#define ValueSetUInt(property, new) property.Value.AsUnsigned = new
		#define ValueSetFloat(property, new) property.Value.AsFloat = new
		#define ValueAsBoolean(property) property.Value.AsBoolean
		#define ValueSetBoolean(property, new) property.Value.AsBoolean = new

		#define AsSigned(property) property.Value.AsSigned
		#define AsUnsigned(property) property.Value.AsUnsigned
		#define AsFloat(property) property.Value.AsFloat
		#define SetSigned(property, new) property.Value.AsSigned = new
		#define SetUnsigned(property, new) property.Value.AsUnsigned = new
		#define SetFloat(property, new) property.Value.AsFloat = new

		#define AsBoolean(property) property.Value.AsBoolean
		#define SetBoolean(property, new) property.Value.AsBoolean = new

		#define AsString(property) property.Value
		#define SetString(property, new) property.Value = new

		/* Rethink all of this registering crap - simple is best */
		inline void UnregisterSharedProperty(std::string Label)
		{
			std::string key = Label;
			transform (key.begin(), key.end(), key.begin(), (int(*)(int))std::toupper);

			m_State.UpdateData.erase(m_State.Properties[key]);
			m_State.Order.erase(std::remove(m_State.Order.begin(), m_State.Order.end(), m_State.Properties[key]),
			                    m_State.Order.end());
			m_State.Properties.erase(key);
		}

		inline void RegisterSharedPropertyWithKey(Property &Initializer, std::string Key,
		                                          std::string Label, std::string Hint)
		{
			transform (Key.begin(), Key.end(), Key.begin(), (int(*)(int))std::toupper);

			assert(m_State.Properties[Key]==NULL);

			m_State.Properties[Key] = &Initializer;
			m_State.Order.push_back(&Initializer);
			Initializer.SetKey(Key);
			Initializer.SetLabel(Label);
			Initializer.SetHint(Hint);

			if (m_AudioCH && !Initializer.IsInternal())
				Initializer.RegisterChannel(*m_AudioCH, Label);
		}

		inline void RegisterSharedProperty(Property &Initializer, std::string Label, std::string Hint)
		{
			RegisterSharedPropertyWithKey(Initializer, Label, Label, Hint);
		}
	};
}
}

#endif
