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
#ifndef SPIRAL_DATA_BINDING_H
#define SPIRAL_DATA_BINDING_H

#include "DataSource.h"

#include <string>
#include <vector>

namespace Spiral
{
	namespace Data
	{
		struct Attribute
		{
			const char *Name;
			ControlValue::Type Type;
			bool Notifies;
		};

		class Bindable;

		class BindableObserver
		{
		public:
			virtual ~BindableObserver() {}
			virtual void BindableChanged(Bindable &bindable, const Attribute &attribute) = 0;
			virtual void BindableDestroyed(Bindable &bindable) = 0;
		};

		class Bindable
		{
		public:
			Bindable();
			virtual ~Bindable();

			virtual const Attribute *FindAttribute(const std::string &name) const = 0;
			virtual const Attribute *ValueAttribute() const = 0;
			virtual bool GetAttribute(const Attribute &attribute, ControlValue &out) const = 0;
			virtual bool SetAttribute(const Attribute &attribute, const ControlValue &value) = 0;

			void Observe(BindableObserver *observer);
			void Unobserve(BindableObserver *observer);

		protected:
			void Changed(const Attribute &attribute);

		private:
			std::vector<BindableObserver *> observers;
			Bindable(const Bindable &);
			Bindable &operator=(const Bindable &);
		};

		class ValueConverter
		{
		public:
			virtual ~ValueConverter() {}
			virtual bool ToControl(const ControlValue &field, ControlValue::Type to, ControlValue &out) const;
			virtual bool ToField(const ControlValue &control, ControlValue::Type to, ControlValue &out) const;
		};

		class Binding: public BindableObserver, public DataObserver
		{
		public:
			Bindable *Target;
			const Attribute *Endpoint;
			DataSource *Source;
			int Field;
			bool Writes;
			bool Wrote;
			ValueConverter *Converter;

			Binding(Bindable *target, const Attribute *endpoint, DataSource *source, int field, bool writes, ValueConverter *converter = NULL);
			virtual ~Binding();

			void Refresh();

			virtual void BindableChanged(Bindable &bindable, const Attribute &attribute);
			virtual void BindableDestroyed(Bindable &bindable);
			virtual void DataChanged(DataSource &source, int field);
			virtual void DataSourceDestroyed(DataSource &source);

			static bool Convert(const ControlValue &from, ControlValue::Type to, ControlValue &out);
			static std::string Format(const ControlValue &value);

		private:
			bool refreshing;
			Binding(const Binding &);
			Binding &operator=(const Binding &);
		};

		class Binder
		{
		public:
			Binder();
			~Binder();

			Binding *Bind(Bindable *target, const char *attribute, DataSource *source, const std::string &field, ValueConverter *converter = NULL);
			Binding *BindValue(Bindable *target, DataSource *source, const std::string &field, ValueConverter *converter = NULL);
			void Unbind(Bindable *target);
			void Clear();
			void Refresh();

			size_t Count() const { return bindings.size(); }
			Binding *At(size_t i) const { return bindings[i]; }

		private:
			std::vector<Binding *> bindings;
			Binder(const Binder &);
			Binder &operator=(const Binder &);
		};
	}
}

#endif
