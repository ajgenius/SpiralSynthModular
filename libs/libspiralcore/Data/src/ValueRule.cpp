// SPDX-License-Identifier: GPL-2.0-or-later
#include "ValueRule.h"
#include "JSON.h"
#include <cstdlib>

namespace Spiral
{
	namespace Data
	{
		namespace
		{
			bool Number(const Spumoni::JSON *node, double &out)
			{
				if (!node || node->GetType() != Spumoni::JSON::Number)
					return false;
				out = std::atof(node->Text().c_str());
				return true;
			}

			bool Refuse(std::string &problem, const std::string &why)
			{
				problem = why;
				return false;
			}
		}

		bool ValueRule::Text::Parse(const Spumoni::JSON *node, Text &out)
		{
			out = Text();
			if (node && node->GetType() == Spumoni::JSON::String)
			{
				out.fallback = node->Text();
				return true;
			}
			if (!node || node->GetType() != Spumoni::JSON::Object || !node->Get("default") ||
			    node->Get("default")->GetType() != Spumoni::JSON::String)
				return false;
			out.fallback = node->Get("default")->Text();
			if (node->Get("key") && node->Get("key")->GetType() == Spumoni::JSON::String)
				out.key = node->Get("key")->Text();
			const Spumoni::JSON *translations = node->Get("translations");
			if (translations && translations->GetType() == Spumoni::JSON::Object)
			{
				const std::vector<std::string> tags = translations->Keys();
				for (size_t i = 0; i < tags.size(); ++i)
					if (translations->Get(tags[i])->GetType() == Spumoni::JSON::String)
						out.translations.push_back(std::make_pair(tags[i], translations->Get(tags[i])->Text()));
			}
			return true;
		}

		ValueRule::ValueRule():
			m_Kind(Literal),
			m_Literal(true)
		{
		}

		bool ValueRule::Looks(const Spumoni::JSON *node)
		{
			if (!node || node->GetType() != Spumoni::JSON::Object)
				return false;
			return node->Get("property") || node->Get("control") || node->Get("any") || node->Get("all") ||
				node->Get("not");
		}

		bool ValueRule::Parse(const Spumoni::JSON *node, const Scope &scope, ValueRule &out, std::string &problem)
		{
			out = ValueRule();
			if (!node)
				return true;
			if (node->GetType() == Spumoni::JSON::Boolean)
			{
				out.m_Literal = node->AsBool();
				return true;
			}
			if (node->GetType() != Spumoni::JSON::Object)
				return Refuse(problem, "a binding is true, false or an object");

			const std::vector<std::string> keys = node->Keys();
			const bool composite = node->Get("any") || node->Get("all") || node->Get("not");
			if (composite)
			{
				if (keys.size() != 1)
					return Refuse(problem, "any, all and not stand alone");
				const char *which = node->Get("any") ? "any" : node->Get("all") ? "all" : "not";
				out.m_Kind = *which == 'a' && which[1] == 'n' ? Any : *which == 'a' ? All : Not;
				const Spumoni::JSON *inner = node->Get(which);
				if (out.m_Kind == Not)
				{
					ValueRule inside;
					if (!Parse(inner, scope, inside, problem))
						return false;
					out.m_Cases.push_back(inside);
					return true;
				}
				if (inner->GetType() != Spumoni::JSON::Array || inner->Size() == 0)
					return Refuse(problem, std::string(which) + " needs a non-empty array of bindings");
				for (size_t i = 0; i < inner->Size(); ++i)
				{
					ValueRule inside;
					if (!Parse(inner->At(i), scope, inside, problem))
						return false;
					out.m_Cases.push_back(inside);
				}
				return true;
			}

			if (node->Get("control") || node->Get("attribute"))
			{
				const Spumoni::JSON *control = node->Get("control"), *attribute = node->Get("attribute");
				if (keys.size() != 2 || !control || !attribute || control->GetType() != Spumoni::JSON::String ||
				    attribute->GetType() != Spumoni::JSON::String)
					return Refuse(problem, "a control binding is {control, attribute}");
				if (!scope.HasAttribute(control->Text(), attribute->Text()))
					return Refuse(problem, "no control " + control->Text() + " with attribute " + attribute->Text());
				out.m_Kind = Attribute;
				out.m_Control = control->Text();
				out.m_AttributeName = attribute->Text();
				return true;
			}

			const Spumoni::JSON *property = node->Get("property");
			if (!property || property->GetType() != Spumoni::JSON::String)
				return Refuse(problem, "a binding names a property or a control");
			if (!scope.HasField(property->Text()))
				return Refuse(problem, "unknown property " + property->Text());
			out.m_Property = property->Text();
			out.m_Kind = Truth;
			for (size_t i = 0; i < keys.size(); ++i)
			{
				const std::string &key = keys[i];
				const Spumoni::JSON *value = node->Get(key);
				double number;
				if (key == "property")
					continue;
				if (key == "equals" || key == "not_equals")
				{
					if (out.m_Kind != Truth)
						return Refuse(problem, "one comparison per binding");
					if (!Number(value, number))
						return Refuse(problem, key + " needs a number");
					out.m_Kind = key == "equals" ? Equals : NotEquals;
					out.m_Values.push_back(number);
				}
				else if (key == "one_of")
				{
					if (out.m_Kind != Truth)
						return Refuse(problem, "one comparison per binding");
					if (value->GetType() != Spumoni::JSON::Array || value->Size() == 0)
						return Refuse(problem, "one_of needs a non-empty array of numbers");
					for (size_t j = 0; j < value->Size(); ++j)
					{
						if (!Number(value->At(j), number))
							return Refuse(problem, "one_of needs numbers");
						out.m_Values.push_back(number);
					}
					out.m_Kind = OneOf;
				}
				else if (key == "on" || key == "off")
				{
					if (out.m_Kind != Truth && out.m_Kind != Select)
						return Refuse(problem, "on/off do not combine with a comparison");
					if (!Text::Parse(value, key == "on" ? out.m_On : out.m_Off))
						return Refuse(problem, key + " needs a text");
					out.m_Kind = Select;
				}
				else
					return Refuse(problem, "unknown key " + key + " in a binding");
			}
			if (out.m_Kind == Select && (!node->Get("on") || !node->Get("off")))
				return Refuse(problem, "a selected text needs both on and off");
			return true;
		}

		bool ValueRule::Verify(const Scope &scope, std::string &problem) const
		{
			if (m_Kind == Attribute && !scope.HasAttribute(m_Control, m_AttributeName))
				return Refuse(problem, "no control " + m_Control + " with attribute " + m_AttributeName);
			for (size_t i = 0; i < m_Cases.size(); ++i)
				if (!m_Cases[i].Verify(scope, problem))
					return false;
			return true;
		}

		ControlValue ValueRule::Evaluate(const Scope &scope) const
		{
			switch (m_Kind)
			{
			case Literal:
				return ControlValue(m_Literal);
			case Truth:
				return ControlValue(scope.Field(m_Property) != 0);
			case Equals:
				return ControlValue(scope.Field(m_Property) == m_Values[0]);
			case NotEquals:
				return ControlValue(scope.Field(m_Property) != m_Values[0]);
			case OneOf:
			{
				const double value = scope.Field(m_Property);
				for (size_t i = 0; i < m_Values.size(); ++i)
					if (value == m_Values[i])
						return ControlValue(true);
				return ControlValue(false);
			}
			case Select:
				return ControlValue(scope.Localize(scope.Field(m_Property) != 0 ? m_On : m_Off));
			case Attribute:
				return ControlValue(scope.Attribute(m_Control, m_AttributeName));
			case Any:
				for (size_t i = 0; i < m_Cases.size(); ++i)
					if (m_Cases[i].Holds(scope))
						return ControlValue(true);
				return ControlValue(false);
			case All:
				for (size_t i = 0; i < m_Cases.size(); ++i)
					if (!m_Cases[i].Holds(scope))
						return ControlValue(false);
				return ControlValue(true);
			case Not:
				return ControlValue(!m_Cases[0].Holds(scope));
			}
			return ControlValue(false);
		}
	}
}
