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
#include "MemorySource.h"

namespace Spiral
{
	namespace Data
	{
		MemorySource::MemorySource():
		    position(-1)
		{
		}

		int MemorySource::AddField(const std::string &key, ControlValue::Type type, bool readOnly, const std::string &label)
		{
			fields.push_back(DataField(key, type, readOnly, label));
			return (int)fields.size() - 1;
		}

		long MemorySource::AddRow(const std::vector<ControlValue> &values)
		{
			std::vector<ControlValue> row = values;
			row.resize(fields.size());
			rows.push_back(row);
			return (long)rows.size() - 1;
		}

		int MemorySource::FieldCount() const
		{
			return (int)fields.size();
		}

		const DataField &MemorySource::Field(int index) const
		{
			return fields[index];
		}

		bool MemorySource::Get(int field, ControlValue &out) const
		{
			if (position < 0 || position >= (long)rows.size() || field < 0 || field >= (int)fields.size())
				return false;
			out = rows[position][field];
			return true;
		}

		bool MemorySource::Set(int field, const ControlValue &value)
		{
			if (position < 0 || position >= (long)rows.size() || field < 0 || field >= (int)fields.size() ||
			    fields[field].ReadOnly)
				return false;
			rows[position][field] = value;
			Changed(field);
			return true;
		}

		long MemorySource::Count() const
		{
			return (long)rows.size();
		}

		long MemorySource::Position() const
		{
			return position;
		}

		bool MemorySource::Seek(long row)
		{
			if (row < 0 || row >= (long)rows.size())
				return false;
			position = row;
			Changed(-1);
			return true;
		}
	}
}
