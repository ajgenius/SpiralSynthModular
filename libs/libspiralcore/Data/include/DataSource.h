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
#ifndef SPIRAL_DATA_DATA_SOURCE_H
#define SPIRAL_DATA_DATA_SOURCE_H

#include <string>
#include <vector>

namespace Spiral
{
	namespace Data
	{
		struct ControlValue
		{
			enum Type
			{
				Number,
				Integer,
				Boolean,
				String
			} type;
			double number;
			bool boolean;
			std::string text;

			explicit ControlValue(double v = 0):
			    type(Number),
			    number(v),
			    boolean(false)
			{
			}
			explicit ControlValue(bool v):
			    type(Boolean),
			    number(v ? 1 : 0),
			    boolean(v)
			{
			}
			explicit ControlValue(const std::string &v):
			    type(String),
			    number(0),
			    boolean(false),
			    text(v)
			{
			}
		};

		struct DataField
		{
			std::string Key;
			ControlValue::Type Type;
			bool ReadOnly;
			std::string Label;

			DataField(): Type(ControlValue::String), ReadOnly(true) {}
			DataField(const std::string &key, ControlValue::Type type, bool readOnly = true, const std::string &label = std::string()):
			    Key(key), Type(type), ReadOnly(readOnly), Label(label) {}
		};

		class DataSource;

		class DataObserver
		{
		public:
			virtual ~DataObserver() {}
			virtual void DataChanged(DataSource &source, int field) = 0;
			virtual void DataSourceDestroyed(DataSource &source) = 0;
		};

		class DataSource
		{
		public:
			DataSource();
			virtual ~DataSource();

			virtual int FieldCount() const = 0;
			virtual const DataField &Field(int index) const = 0;
			int FindField(const std::string &key) const;

			virtual bool Get(int field, ControlValue &out) const = 0;
			virtual bool Set(int field, const ControlValue &value);

			virtual long Count() const;
			virtual long Measure();
			virtual long Position() const = 0;
			virtual bool Seek(long row) = 0;
			bool First();
			bool Prev();
			bool Next();
			virtual bool Last();
			bool AtFirst() const;
			bool AtLast() const;

			void Observe(DataObserver *observer);
			void Unobserve(DataObserver *observer);

		protected:
			void Changed(int field);

		private:
			std::vector<DataObserver *> observers;
		};
	}
}

#endif
