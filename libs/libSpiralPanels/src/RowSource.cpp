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
// Rows over a flat source's numbered fields.
#include "RowSource.h"
#include <algorithm>
#include <cstdio>

namespace Spiral
{
	namespace Data
	{
		namespace
		{
			// Bound only the search for unnamed, numbered properties.
			const long kMostRows = 4096;

			bool Whole(const ControlValue &value, long &out)
			{
				if (value.type == ControlValue::String)
					return false;

				out = value.type == ControlValue::Boolean ? (value.boolean ? 1 : 0) : (long)value.number;

				return true;
			}
		}

		RowSource::RowSource(DataSource *source, long firstNumber):
		    flat(source),
		    first(firstNumber),
		    fixedCount(-1),
		    countField(-1),
		    positionField(-1),
		    position(0),
		    seenPosition(-1),
		    seenCount(-1),
		    table(NULL),
		    columns(0), revision(1)
		{
			if (flat)
				flat->Observe(this);
		}

		RowSource::~RowSource()
		{
			if (flat)
				flat->Unobserve(this);

			delete table;
		}

		void RowSource::TableFrom(TableFeed *feed, const std::vector<std::string> &names)
		{
			delete table;
			table = feed;
			columns = names.empty() ? 1 : (int)names.size();
			fields.clear();

			for (int c = 0; c < columns; ++c)
				fields.push_back(DataField(c < (int)names.size() ? names[c] : std::string("0"), ControlValue::Number));

			cells.clear();

			if (table)
				table->Cells(cells);
		}

		void RowSource::TableFrom(TableFeed *feed, int width)
		{
			std::vector<std::string> names;

			for (int c = 0; c < width; ++c)
			{
				char number[32];

				std::snprintf(number, sizeof(number), "%d", c);
				names.push_back(number);
			}

			TableFrom(feed, names);
		}

		int RowSource::FlatField(long row, int field) const
		{
			if (!flat || row < 0 || field < 0 || field >= (int)fields.size())
				return -1;

			char number[32];

			std::snprintf(number, sizeof(number), "%ld", first + row);

			return flat->FindField(fields[field].Key + number);
		}

		long RowSource::Present(long limit) const
		{
			// A table has the rows it was handed.
			if (table)
				return std::min(limit, (long)(cells.size() / columns));

			long rows = 0;

			while (rows < limit && !fields.empty())
			{
				for (int f = 0; f < (int)fields.size(); ++f)
					if (FlatField(rows, f) < 0)
						return rows;

				++rows;
			}

			return rows;
		}

		int RowSource::AddField(const std::string &base)
		{
			fields.push_back(DataField(base, ControlValue::Number));

			const int added = (int)fields.size() - 1;
			const int source = FlatField(0, added);

			if (source < 0)
			{
				fields.pop_back();

				return -1;
			}

			fields[added] = flat->Field(source);
			fields[added].Key = base;

			return added;
		}

		void RowSource::SetCount(long rows)
		{
			fixedCount = rows;
			countField = -1;
		}

		bool RowSource::CountFrom(const std::string &field)
		{
			const int found = flat ? flat->FindField(field) : -1;

			if (found < 0)
				return false;

			countField = found;
			fixedCount = -1;

			return true;
		}

		bool RowSource::PositionFrom(const std::string &field)
		{
			const int found = flat ? flat->FindField(field) : -1;

			if (found < 0)
				return false;

			positionField = found;

			return true;
		}

		int RowSource::FieldCount() const
		{
			return (int)fields.size();
		}

		const DataField &RowSource::Field(int index) const
		{
			return fields[index];
		}

		long RowSource::Count() const
		{
			long limit = fixedCount;
			if (limit < 0)
				limit = table ? (long)(cells.size() / columns) : kMostRows;
			ControlValue held;

			if (countField >= 0 && flat && flat->Get(countField, held))
				Whole(held, limit);

			return Present(limit < 0 ? 0 : limit);
		}

		long RowSource::Position() const
		{
			long row = position;
			ControlValue held;

			if (positionField >= 0 && flat && flat->Get(positionField, held))
				Whole(held, row);

			return row;
		}

		bool RowSource::Seek(long row)
		{
			if (row < 0 || row >= Count())
				return false;

			// A cursor kept in a field is moved by writing the field, so
			// whatever else is bound to it follows.
			if (positionField >= 0)
			{
				if (!flat->Set(positionField, ControlValue((double)row)))
					return false;
			}
			else
				position = row;

			seenPosition = row;
			Changed(-1);

			return true;
		}

		bool RowSource::GetAt(long row, int field, ControlValue &out) const
		{
			if (table)
			{
				if (row < 0 || row >= Count() || field < 0 || field >= columns)
					return false;

				out = ControlValue(cells[(size_t)row * columns + field]);

				return true;
			}

			const int source = row < Count() ? FlatField(row, field) : -1;

			return source >= 0 && flat->Get(source, out);
		}

		bool RowSource::SetAt(long row, int field, const ControlValue &value)
		{
			// A table's cell is changed by a command to the device.
			if (table)
				return false;

			const int source = row < Count() ? FlatField(row, field) : -1;

			if (source < 0 || fields[field].ReadOnly || !flat->Set(source, value))
				return false;

			if (row == Position())
				Changed(field);

			return true;
		}

		bool RowSource::Get(int field, ControlValue &out) const
		{
			return GetAt(Position(), field, out);
		}

		bool RowSource::Set(int field, const ControlValue &value)
		{
			return SetAt(Position(), field, value);
		}

		bool RowSource::Poll()
		{
			std::vector<double> now;
			const bool edited = table && table->Cells(now) && now != cells;

			if (edited)
			{
				cells.swap(now);
				++revision;
			}

			const long row = Position(), rows = Count();
			const bool moved = edited || row != seenPosition || rows != seenCount;

			seenPosition = row;
			seenCount = rows;

			if (moved)
				Changed(-1);

			return moved;
		}

		void RowSource::DataChanged(DataSource &, int field)
		{
			// The flat row reloaded, the cursor's field moved or the count
			// changed: every field here may show something else.
			if (field < 0 || field == positionField || field == countField)
			{
				seenPosition = Position();
				seenCount = Count();
				Changed(-1);

				return;
			}

			// A table's cells are none of the flat source's fields.
			if (table)
				return;

			const long row = Position();

			for (int f = 0; f < (int)fields.size(); ++f)
				if (FlatField(row, f) == field)
					Changed(f);
		}

		void RowSource::DataSourceDestroyed(DataSource &)
		{
			flat = NULL;
			Changed(-1);
		}
	}
}
