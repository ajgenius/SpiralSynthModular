// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef SPIRAL_PANELS_DEVICE_SOURCE_H
#define SPIRAL_PANELS_DEVICE_SOURCE_H

#include "PropertySource.h"
#include "PanelProperty.h"
#include "PanelSource.h"
#include <map>
#include <string>
#include <vector>

class SpiralPlugin;

namespace Spiral
{
namespace Core
{
	class Property;
}
}

namespace Spiral
{
	namespace Panels
	{
		class DeviceSource: public Spiral::Data::PropertySource, public PanelSource
		{
		public:
			explicit DeviceSource(SpiralPlugin *plugin);

			Spiral::Data::DataSource &Data() { return *this; }
			void Collect();
			void Refresh();
			bool AddElement(const PanelProperty &property);
			virtual int FieldCount() const;
			virtual const Spiral::Data::DataField &Field(int index) const;
			virtual bool Get(int field, Spiral::Data::ControlValue &out) const;
			virtual bool Set(int field, const Spiral::Data::ControlValue &value);

			void Describe(std::map<std::string, PanelProperty *> &properties) const;
			bool Write(const std::string &key, double value);
			bool WriteText(const std::string &key, const std::string &text);
			bool ReadNow(const PanelProperty &property, double &value) const;
			bool Command(const std::string &name, const std::vector<std::string> &args);
			bool ReadBytes(const std::string &channel, std::vector<unsigned char> &bytes) const;
			bool ReadFloats(const std::string &channel, std::vector<float> &values) const;
			bool ReadTable(const std::string &channel, std::vector<double> &cells) const;
			bool FetchFloats(const std::string &command, const std::string &sizeChannel, const std::string &bufferChannel, std::vector<float> &values);
			void Wait();
			Feed *RequestFeed(bool output, int port);
			void ReleaseFeed(Feed *feed);
			unsigned Colour() const;
			SpiralPlugin *Plugin() const { return m_Plugin; }

		protected:
			static PanelProperty *FromRegistered(Spiral::Core::Property &property);
			struct Declared
			{
				std::vector<std::string> Args;
				std::vector<PanelProperty::Kind> Kinds;
				std::vector<bool> Typed;
			};
			struct Element
			{
				Spiral::Data::DataField Field;
				std::string Channel;
				int Offset;
				int Size;
				Spiral::Data::ControlValue Value;
				bool Known;
			};
			bool ReadElement(const Element &element, Spiral::Data::ControlValue &out) const;
			bool WriteElement(const Element &element, const Spiral::Data::ControlValue &value);
			std::vector<Element> m_Elements;
			std::string ChannelNamed(const std::string &name) const;
			SpiralPlugin *m_Plugin;
			std::map<std::string, Declared> m_Commands;
		};
	}
}

#endif
