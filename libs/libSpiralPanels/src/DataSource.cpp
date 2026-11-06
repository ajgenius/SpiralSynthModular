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
#include "DataSource.h"

#include <algorithm>

namespace Spiral
{
	namespace Data
	{
		DataSource::DataSource()
		{
		}

		DataSource::~DataSource()
		{
			while (!observers.empty())
			{
				DataObserver *observer = observers.back();
				observers.pop_back();
				observer->DataSourceDestroyed(*this);
			}
		}

		int DataSource::FindField(const std::string &key) const
		{
			for (int i = 0; i < FieldCount(); ++i)
				if (Field(i).Key == key)
					return i;
			return -1;
		}

		bool DataSource::Set(int, const ControlValue &)
		{
			return false;
		}

		long DataSource::Count() const
		{
			return -1;
		}

		long DataSource::Measure()
		{
			return Count();
		}

		bool DataSource::First()
		{
			return Seek(0);
		}

		bool DataSource::Prev()
		{
			return Position() > 0 && Seek(Position() - 1);
		}

		bool DataSource::Next()
		{
			return Seek(Position() + 1);
		}

		bool DataSource::Last()
		{
			const long count = Measure();
			if (count >= 0)
				return count > 0 && Seek(count - 1);
			long row = Position() < 0 ? 0 : Position();
			if (!Seek(row))
				return false;
			while (Seek(row + 1))
				++row;
			return Seek(row);
		}

		bool DataSource::AtFirst() const
		{
			return Position() <= 0;
		}

		bool DataSource::AtLast() const
		{
			return Count() >= 0 && Position() >= Count() - 1;
		}

		void DataSource::Observe(DataObserver *observer)
		{
			if (observer && std::find(observers.begin(), observers.end(), observer) == observers.end())
				observers.push_back(observer);
		}

		void DataSource::Unobserve(DataObserver *observer)
		{
			observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
		}

		void DataSource::Changed(int field)
		{
			const std::vector<DataObserver *> snapshot = observers;
			for (size_t i = 0; i < snapshot.size(); ++i)
				if (std::find(observers.begin(), observers.end(), snapshot[i]) != observers.end())
					snapshot[i]->DataChanged(*this, field);
		}
	}
}
