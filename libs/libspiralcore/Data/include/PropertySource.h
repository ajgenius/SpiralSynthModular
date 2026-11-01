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
#ifndef SPIRAL_DATA_PROPERTY_SOURCE_H
#define SPIRAL_DATA_PROPERTY_SOURCE_H

#include "DataSource.h"

namespace Spiral
{
	namespace Core
	{
		class ChannelHandler;
		class Device;
		class Property;
	}

	namespace Data
	{
		class PropertySource: public DataSource
		{
		public:
			PropertySource(Core::Device *device, Core::ChannelHandler *channels);
			virtual ~PropertySource();

			void Collect();
			void Refresh();
			Core::Property *Describe(int field) const;
			bool ReadNow(int field, ControlValue &out) const;

			virtual int FieldCount() const;
			virtual const DataField &Field(int index) const;
			virtual bool Get(int field, ControlValue &out) const;
			virtual bool Set(int field, const ControlValue &value);
			virtual long Count() const;
			virtual long Position() const;
			virtual bool Seek(long row);

		protected:
			Core::Device *m_Device;
			Core::ChannelHandler *m_Channels;

		private:
			enum Storage
			{
				Float,
				Signed,
				Unsigned,
				Toggle,
				Text
			};

			struct Slot
			{
				DataField Field;
				Core::Property *Property;
				std::string Channel;
				int Size;
				Storage Kind;
				ControlValue Value;
				bool Known;
			};

			bool Read(const Slot &slot, ControlValue &out) const;
			bool Store(Slot &slot, ControlValue &value);
			void Release();

			std::vector<Slot> m_Slots;

			PropertySource(const PropertySource &);
			PropertySource &operator=(const PropertySource &);
		};
	}
}

#endif
