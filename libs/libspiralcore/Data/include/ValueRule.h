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
			struct Text
			{
				std::string key, fallback;
				std::vector<std::pair<std::string, std::string> > translations;
				static bool Parse(const Spumoni::JSON *node, Text &out);
			};

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
				Literal,
				Truth,
				Equals,
				NotEquals,
				OneOf,
				Select,
				Attribute,
				Any,
				All,
				Not
			};

			ValueRule();

			static bool Parse(const Spumoni::JSON *node, const Scope &scope, ValueRule &out, std::string &problem);
			static bool Looks(const Spumoni::JSON *node);

			bool Verify(const Scope &scope, std::string &problem) const;
			ControlValue Evaluate(const Scope &scope) const;
			bool Holds(const Scope &scope) const { return Evaluate(scope).boolean; }
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
