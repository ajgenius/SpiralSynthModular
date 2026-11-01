// What a control does when something happens, as a panel file writes it:
// `on` maps an event the control declares to the actions it runs, each
// one verb — write a property (or send a command with arguments), bump
// one by a delta, set another control's attribute, clear a control's
// indicators. Parsed once against a scope, which also decides what is
// valid (an unknown event, property, control or argument is refused at
// load), then fired by name with the event's own arguments.
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
			// The event's own arguments, by the names its declaration gives them.
			typedef std::map<std::string, ControlValue> Arguments;

			// Where a handler reads from and what it may name: the panel's
			// properties, its controls' attributes and indicators, and the
			// events this control declares with their argument names.
			class Scope
			{
			public:
				virtual ~Scope() {}
				virtual bool HasField(const std::string &name) const = 0;
				virtual bool HasControl(const std::string &name) const = 0;
				// A readable attribute, for an argument; a settable one, for set.
				virtual bool HasAttribute(const std::string &control, const std::string &attribute) const = 0;
				virtual bool CanSetAttribute(const std::string &control, const std::string &attribute) const = 0;
				// False when the control does not declare the event; else the
				// names of the arguments it carries, in order.
				virtual bool Event(const std::string &name, std::vector<std::string> &arguments) const = 0;
			};

			// What a fired handler does to the panel.
			class Sink
			{
			public:
				virtual ~Sink() {}
				virtual void Write(const std::string &property, const ControlValue &value,
						   const std::vector<ControlValue> &arguments) = 0;
				// Bumps the property and returns what it now holds; the actions
				// after a delta see that as the event's value, which is how a
				// nudge button sends a command with the bumped value.
				virtual double Delta(const std::string &property, double amount, double factor, bool byFactor) = 0;
				virtual void Set(const std::string &control, const std::string &attribute, const ControlValue &value) = 0;
				virtual void Clear(const std::string &control) = 0;
				virtual double Attribute(const std::string &control, const std::string &attribute) const = 0;
			};

			// An argument passed with a write: the event's own by name, a
			// constant number, or another control's attribute now.
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
					Write,   // {write: PROPERTY, value?, with?}
					Delta,   // {delta: PROPERTY | [PROPERTIES], amount | factor}
					Set,     // {set: CONTROL, attribute}
					Clear    // {clear: [CONTROLS]}
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

			// False with a reason when the `on` node is not a map of declared
			// events to actions this scope can serve. A missing node is empty.
			static bool Parse(const Spumoni::JSON *node, const Scope &scope, Handlers &out, std::string &problem);
			// One handler list read the older way: the actions `write`-style
			// entries spell, appended under `event`.
			bool Add(const std::string &event, const Actions &actions, const Scope &scope, std::string &problem);
			static bool ParseWrites(const Spumoni::JSON *list, const Scope &scope, const std::string &event,
						Actions &out, std::string &problem);

			bool Empty() const { return m_Events.empty(); }
			bool Has(const std::string &event) const { return m_Events.find(event) != m_Events.end(); }
			std::vector<std::string> Events() const;
			const Actions &For(const std::string &event) const;

			// False with a reason when a set, clear or attribute argument names a
			// control or attribute the scope does not have now: a loader that
			// trusted them while the panel was still being built checks here.
			bool Verify(const Scope &scope, std::string &problem) const;

			// Runs the event's actions in order against the sink; a named
			// argument the event did not carry reads as the number 0, and a
			// delta's result is the value the actions after it see.
			void Fire(const std::string &event, const Arguments &arguments, Sink &sink) const;

		private:
			static bool ParseAction(const Spumoni::JSON *node, const Scope &scope, const std::string &event,
						const std::vector<std::string> &carried, Action &out, std::string &problem);
			std::map<std::string, Actions> m_Events;
		};
	}
}
#endif
