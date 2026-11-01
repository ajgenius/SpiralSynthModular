// How a control attribute's value is taken from state, as a panel file
// writes it: a literal, a property's truth or a comparison on it, a text
// selected by a property's truth, another control's attribute, or a
// composition with any/all/not. Parsed once against a scope, which also
// decides what is valid (an unknown property or control is refused at
// load), then evaluated whenever the state may have moved.
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_DATA_VALUE_RULE_H
#define SPIRAL_DATA_VALUE_RULE_H

#include "DataSource.h"
#include <string>
#include <utility>
#include <vector>

namespace Spumoni
{
	class JSON;
}

namespace Spiral
{
	namespace Data
	{
		class ValueRule
		{
		public:
			// A text as a panel file writes it: a string, or an object with the
			// `default` and, optionally, a translation `key` and `translations`
			// by language tag, in the file's order. Which one shows is the
			// host's decision, made when the rule is evaluated.
			struct Text
			{
				std::string key, fallback;
				std::vector<std::pair<std::string, std::string> > translations;
				static bool Parse(const Spumoni::JSON *node, Text &out);
			};

			// Where a rule reads from. Fields are the numeric properties the
			// panel can see; attributes belong to the panel's other controls;
			// a text is localized by the host, the fallback failing that.
			class Scope
			{
			public:
				virtual ~Scope() {}
				virtual bool HasField(const std::string &name) const = 0;
				virtual double Field(const std::string &name) const = 0;
				virtual bool HasAttribute(const std::string &control, const std::string &attribute) const = 0;
				virtual double Attribute(const std::string &control, const std::string &attribute) const = 0;
				virtual std::string Localize(const Text &text) const { return text.fallback; }
			};

			enum Kind
			{
				Literal,     // true/false as written
				Truth,       // {property}: nonzero
				Equals,      // {property, equals}
				NotEquals,   // {property, not_equals}
				OneOf,       // {property, one_of: [...]}
				Select,      // {property, on, off}: a text by the property's truth
				Attribute,   // {control, attribute}: that control's value
				Any,         // {any: [...]}
				All,         // {all: [...]}
				Not          // {not: ...}
			};

			ValueRule();

			// False with a reason when the node is not a binding this scope can
			// serve. A missing node is the literal `true`.
			static bool Parse(const Spumoni::JSON *node, const Scope &scope, ValueRule &out, std::string &problem);
			// True when the node has the shape of a binding object at all, so a
			// loader can tell a binding from an ordinary object value.
			static bool Looks(const Spumoni::JSON *node);

			// False with a reason when a control binding, at any depth, names a
			// control or attribute the scope does not have now.
			bool Verify(const Scope &scope, std::string &problem) const;
			ControlValue Evaluate(const Scope &scope) const;
			bool Holds(const Scope &scope) const { return Evaluate(scope).boolean; }
			// A rule that yields a text (on/off) serves a text attribute and
			// nothing else; every other rule yields a truth or a number.
			bool IsText() const { return m_Kind == Select; }
			Kind Type() const { return m_Kind; }
			const std::string &Property() const { return m_Property; }

		private:
			Kind m_Kind;
			bool m_Literal;
			std::string m_Property;
			std::string m_Control;
			std::string m_AttributeName;
			std::vector<double> m_Values;
			Text m_On, m_Off;
			std::vector<ValueRule> m_Cases;
		};
	}
}
#endif
