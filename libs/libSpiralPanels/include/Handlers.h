// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_DATA_HANDLERS_H
#define SPIRAL_DATA_HANDLERS_H

#include "DataSource.h"
#include <map>
#include <string>
#include <vector>

namespace Spumoni
{
	class JSON;
}

namespace Spiral
{
	namespace Data
	{
		class Handlers
		{
		public:
			typedef std::map<std::string, ControlValue> Arguments;

			class Scope
			{
			public:
				virtual ~Scope() {}
				virtual bool HasField(const std::string &name) const = 0;
				virtual bool HasControl(const std::string &name) const = 0;
				virtual bool HasAttribute(const std::string &control, const std::string &attribute) const = 0;
				virtual bool CanSetAttribute(const std::string &control, const std::string &attribute) const = 0;
				virtual bool Event(const std::string &name, std::vector<std::string> &arguments) const = 0;
			};

			class Sink
			{
			public:
				virtual ~Sink() {}
				virtual void Write(const std::string &property, const ControlValue &value,
						   const std::vector<ControlValue> &arguments) = 0;
				virtual double Delta(const std::string &property, double amount, double factor, bool byFactor) = 0;
				virtual void Set(const std::string &control, const std::string &attribute, const ControlValue &value) = 0;
				virtual void Clear(const std::string &control) = 0;
				virtual double Attribute(const std::string &control, const std::string &attribute) const = 0;
			};

			struct Argument
			{
				enum Kind
				{
					Named,
					Constant,
					Attribute
				};
				Kind kind;
				std::string name;
				double constant;
				std::string control, attribute;
				Argument(): kind(Constant), constant(0) {}
			};

			struct Action
			{
				enum Verb
				{
					Write,
					Delta,
					Set,
					Clear
				};
				Verb verb;
				std::vector<std::string> properties;
				bool hasValue;
				ControlValue value;
				std::vector<Argument> with;
				double amount, factor;
				bool byFactor;
				std::string control, attribute;
				std::vector<std::string> controls;
				Action(): verb(Write), hasValue(false), amount(0), factor(1), byFactor(false) {}
			};
			typedef std::vector<Action> Actions;

			static bool Parse(const Spumoni::JSON *node, const Scope &scope, Handlers &out, std::string &problem);
			bool Add(const std::string &event, const Actions &actions, const Scope &scope, std::string &problem);
			static bool ParseWrites(const Spumoni::JSON *list, const Scope &scope, const std::string &event,
						Actions &out, std::string &problem);

			bool Empty() const { return m_Events.empty(); }
			bool Has(const std::string &event) const { return m_Events.find(event) != m_Events.end(); }
			std::vector<std::string> Events() const;
			const Actions &For(const std::string &event) const;

			bool Verify(const Scope &scope, std::string &problem) const;

			void Fire(const std::string &event, const Arguments &arguments, Sink &sink) const;

		private:
			static bool ParseAction(const Spumoni::JSON *node, const Scope &scope, const std::string &event,
						const std::vector<std::string> &carried, Action &out, std::string &problem);
			std::map<std::string, Actions> m_Events;
		};
	}
}
#endif
