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
#ifndef SPIRAL_DATA_MEMORY_SOURCE_H
#define SPIRAL_DATA_MEMORY_SOURCE_H

#include "DataSource.h"

namespace Spiral
{
	namespace Data
	{
		class MemorySource: public DataSource
		{
		public:
			MemorySource();

			int AddField(const std::string &key, ControlValue::Type type, bool readOnly = false, const std::string &label = std::string());
			long AddRow(const std::vector<ControlValue> &values);

			virtual int FieldCount() const;
			virtual const DataField &Field(int index) const;
			virtual bool Get(int field, ControlValue &out) const;
			virtual bool Set(int field, const ControlValue &value);
			virtual long Count() const;
			virtual long Position() const;
			virtual bool Seek(long row);

		private:
			std::vector<DataField> fields;
			std::vector<std::vector<ControlValue> > rows;
			long position;
		};
	}
}

#endif
