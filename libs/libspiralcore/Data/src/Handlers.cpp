// SPDX-License-Identifier: GPL-2.0-or-later
#include "Handlers.h"
#include "JSON.h"
#include <cstdlib>

namespace Spiral
{
	namespace Data
	{
		namespace
		{
			bool Refuse(std::string &problem, const std::string &why)
			{
				problem = why;
				return false;
			}

			bool IsString(const Spumoni::JSON *node)
			{
				return node && node->GetType() == Spumoni::JSON::String;
			}

			bool IsNumber(const Spumoni::JSON *node)
			{
				return node && node->GetType() == Spumoni::JSON::Number;
			}

			bool Contains(const std::vector<std::string> &names, const std::string &name)
			{
				for (size_t i = 0; i < names.size(); ++i)
					if (names[i] == name)
						return true;
				return false;
			}

			// The property names a write or delta names: one, or a list for delta.
			bool Properties(const Spumoni::JSON *node, const Handlers::Scope &scope, bool list,
					std::vector<std::string> &out, std::string &problem)
			{
				if (IsString(node))
					out.push_back(node->Text());
				else if (list && node && node->GetType() == Spumoni::JSON::Array && node->Size() > 0)
					for (size_t i = 0; i < node->Size(); ++i)
					{
						if (!IsString(node->At(i)))
							return Refuse(problem, "a property list holds names");
						out.push_back(node->At(i)->Text());
					}
				else
					return Refuse(problem, list ? "delta names a property or a list of them" : "write names a property");
				for (size_t i = 0; i < out.size(); ++i)
					if (!scope.HasField(out[i]))
						return Refuse(problem, "unknown property " + out[i]);
				return true;
			}

			// A write's value and arguments, shared by {write, value, with} and
			// the older {property, value, with} entry.
			bool WriteAction(const Spumoni::JSON *value, const Spumoni::JSON *with, const Handlers::Scope &scope,
					 const std::string &event, const std::vector<std::string> &carried, Handlers::Action &out,
					 std::string &problem)
			{
				out.verb = Handlers::Action::Write;
				if (value)
				{
					if (IsNumber(value))
						out.value = ControlValue(std::atof(value->Text().c_str()));
					else if (IsString(value))
						out.value = ControlValue(value->Text());
					else if (value->GetType() == Spumoni::JSON::Boolean)
						out.value = ControlValue(value->AsBool());
					else
						return Refuse(problem, "a written value is a number, a text or a truth");
					out.hasValue = true;
				}
				if (with && with->GetType() != Spumoni::JSON::Array)
					return Refuse(problem, "with is a list of arguments");
				for (size_t i = 0; with && i < with->Size(); ++i)
				{
					const Spumoni::JSON *arg = with->At(i);
					Handlers::Argument a;
					if (IsNumber(arg))
					{
						a.kind = Handlers::Argument::Constant;
						a.constant = std::atof(arg->Text().c_str());
					}
					else if (IsString(arg))
					{
						a.kind = Handlers::Argument::Named;
						a.name = arg->Text();
						if (!Contains(carried, a.name))
							return Refuse(problem, event + " carries no argument " + a.name);
					}
					else if (arg && arg->GetType() == Spumoni::JSON::Object && IsString(arg->Get("control")) &&
						 IsString(arg->Get("attribute")) && arg->Keys().size() == 2)
					{
						a.kind = Handlers::Argument::Attribute;
						a.control = arg->Get("control")->Text();
						a.attribute = arg->Get("attribute")->Text();
						if (!scope.HasAttribute(a.control, a.attribute))
							return Refuse(problem, "no control " + a.control + " with attribute " + a.attribute);
					}
					else
						return Refuse(problem, "an argument is a number, an event argument's name or {control, attribute}");
					out.with.push_back(a);
				}
				return true;
			}

			bool AllowedKeys(const Spumoni::JSON *node, const char *const *allowed, std::string &problem)
			{
				const std::vector<std::string> keys = node->Keys();
				for (size_t i = 0; i < keys.size(); ++i)
				{
					bool ok = false;
					for (const char *const *a = allowed; *a; ++a)
						if (keys[i] == *a)
							ok = true;
					if (!ok)
						return Refuse(problem, "unknown key " + keys[i] + " in a " + allowed[0] + " action");
				}
				return true;
			}
		}

		bool Handlers::ParseAction(const Spumoni::JSON *node, const Scope &scope, const std::string &event,
					   const std::vector<std::string> &carried, Action &out, std::string &problem)
		{
			out = Action();
			if (!node || node->GetType() != Spumoni::JSON::Object)
				return Refuse(problem, "an action is an object with one verb");
			const int verbs = (node->Get("write") ? 1 : 0) + (node->Get("delta") ? 1 : 0) + (node->Get("set") ? 1 : 0) +
				(node->Get("clear") ? 1 : 0);
			if (verbs != 1)
				return Refuse(problem, "an action has exactly one verb: write, delta, set or clear");

			if (node->Get("write"))
			{
				static const char *const allowed[] = {"write", "value", "with", NULL};
				if (!AllowedKeys(node, allowed, problem))
					return false;
				if (!Properties(node->Get("write"), scope, false, out.properties, problem))
					return false;
				return WriteAction(node->Get("value"), node->Get("with"), scope, event, carried, out, problem);
			}

			if (node->Get("delta"))
			{
				static const char *const allowed[] = {"delta", "amount", "factor", NULL};
				if (!AllowedKeys(node, allowed, problem))
					return false;
				out.verb = Action::Delta;
				if (!Properties(node->Get("delta"), scope, true, out.properties, problem))
					return false;
				const Spumoni::JSON *amount = node->Get("amount"), *factor = node->Get("factor");
				if ((amount != NULL) == (factor != NULL))
					return Refuse(problem, "delta takes amount or factor, one of them");
				if (amount && !IsNumber(amount))
					return Refuse(problem, "amount is a number");
				if (factor && !IsNumber(factor))
					return Refuse(problem, "factor is a number");
				out.byFactor = factor != NULL;
				out.amount = amount ? std::atof(amount->Text().c_str()) : 0;
				out.factor = factor ? std::atof(factor->Text().c_str()) : 1;
				return true;
			}

			if (node->Get("set"))
			{
				static const char *const allowed[] = {"set", "attribute", NULL};
				if (!AllowedKeys(node, allowed, problem))
					return false;
				out.verb = Action::Set;
				if (!IsString(node->Get("set")) || !IsString(node->Get("attribute")))
					return Refuse(problem, "set names a control and an attribute");
				out.control = node->Get("set")->Text();
				out.attribute = node->Get("attribute")->Text();
				if (!scope.CanSetAttribute(out.control, out.attribute))
					return Refuse(problem, "no control " + out.control + " with a settable " + out.attribute);
				return true;
			}

			static const char *const allowed[] = {"clear", NULL};
			if (!AllowedKeys(node, allowed, problem))
				return false;
			out.verb = Action::Clear;
			const Spumoni::JSON *list = node->Get("clear");
			if (list->GetType() != Spumoni::JSON::Array || list->Size() == 0)
				return Refuse(problem, "clear lists controls");
			for (size_t i = 0; i < list->Size(); ++i)
			{
				if (!IsString(list->At(i)))
					return Refuse(problem, "clear lists control names");
				if (!scope.HasControl(list->At(i)->Text()))
					return Refuse(problem, "no control " + list->At(i)->Text());
				out.controls.push_back(list->At(i)->Text());
			}
			return true;
		}

		bool Handlers::Parse(const Spumoni::JSON *node, const Scope &scope, Handlers &out, std::string &problem)
		{
			out = Handlers();
			if (!node)
				return true;
			if (node->GetType() != Spumoni::JSON::Object)
				return Refuse(problem, "on maps events to their actions");
			const std::vector<std::string> events = node->Keys();
			for (size_t i = 0; i < events.size(); ++i)
			{
				std::vector<std::string> carried;
				if (!scope.Event(events[i], carried))
					return Refuse(problem, "the control declares no event " + events[i]);
				const Spumoni::JSON *list = node->Get(events[i]);
				if (list->GetType() != Spumoni::JSON::Array || list->Size() == 0)
					return Refuse(problem, events[i] + " needs a list of actions");
				Actions &actions = out.m_Events[events[i]];
				for (size_t j = 0; j < list->Size(); ++j)
				{
					Action action;
					if (!ParseAction(list->At(j), scope, events[i], carried, action, problem))
					{
						problem = events[i] + ": " + problem;
						return false;
					}
					actions.push_back(action);
				}
			}
			return true;
		}

		bool Handlers::ParseWrites(const Spumoni::JSON *list, const Scope &scope, const std::string &event,
					   Actions &out, std::string &problem)
		{
			if (!list)
				return true;
			if (list->GetType() != Spumoni::JSON::Array)
				return Refuse(problem, "a write list is an array");
			std::vector<std::string> carried;
			if (!scope.Event(event, carried))
				return Refuse(problem, "the control declares no event " + event);
			for (size_t i = 0; i < list->Size(); ++i)
			{
				// Today's entry spells the verb by its key: {property, value?, with?}.
				const Spumoni::JSON *entry = list->At(i);
				if (!entry || entry->GetType() != Spumoni::JSON::Object || !IsString(entry->Get("property")))
					return Refuse(problem, "a write entry names a property");
				const std::vector<std::string> keys = entry->Keys();
				for (size_t k = 0; k < keys.size(); ++k)
					if (keys[k] != "property" && keys[k] != "value" && keys[k] != "with")
						return Refuse(problem, "unknown key " + keys[k] + " in a write entry");
				Action action;
				if (!Properties(entry->Get("property"), scope, false, action.properties, problem) ||
				    !WriteAction(entry->Get("value"), entry->Get("with"), scope, event, carried, action, problem))
					return false;
				out.push_back(action);
			}
			return true;
		}

		bool Handlers::Add(const std::string &event, const Actions &actions, const Scope &scope, std::string &problem)
		{
			std::vector<std::string> carried;
			if (!scope.Event(event, carried))
				return Refuse(problem, "the control declares no event " + event);
			if (actions.empty())
				return true;
			Actions &mine = m_Events[event];
			mine.insert(mine.end(), actions.begin(), actions.end());
			return true;
		}

		std::vector<std::string> Handlers::Events() const
		{
			std::vector<std::string> names;
			for (std::map<std::string, Actions>::const_iterator it = m_Events.begin(); it != m_Events.end(); ++it)
				names.push_back(it->first);
			return names;
		}

		const Handlers::Actions &Handlers::For(const std::string &event) const
		{
			static const Actions none;
			std::map<std::string, Actions>::const_iterator it = m_Events.find(event);
			return it == m_Events.end() ? none : it->second;
		}

		bool Handlers::Verify(const Scope &scope, std::string &problem) const
		{
			for (std::map<std::string, Actions>::const_iterator it = m_Events.begin(); it != m_Events.end(); ++it)
				for (size_t i = 0; i < it->second.size(); ++i)
				{
					const Action &action = it->second[i];
					if (action.verb == Action::Set && !scope.CanSetAttribute(action.control, action.attribute))
						return Refuse(problem, it->first + ": no control " + action.control + " with a settable " +
							      action.attribute);
					for (size_t c = 0; action.verb == Action::Clear && c < action.controls.size(); ++c)
						if (!scope.HasControl(action.controls[c]))
							return Refuse(problem, it->first + ": no control " + action.controls[c]);
					for (size_t n = 0; action.verb == Action::Write && n < action.with.size(); ++n)
						if (action.with[n].kind == Argument::Attribute &&
						    !scope.HasAttribute(action.with[n].control, action.with[n].attribute))
							return Refuse(problem, it->first + ": no control " + action.with[n].control +
								      " with attribute " + action.with[n].attribute);
				}
			return true;
		}

		void Handlers::Fire(const std::string &event, const Arguments &arguments, Sink &sink) const
		{
			std::map<std::string, Actions>::const_iterator it = m_Events.find(event);
			if (it == m_Events.end())
				return;
			const Arguments::const_iterator own = arguments.find("value");
			ControlValue value = own == arguments.end() ? ControlValue(0.0) : own->second;
			for (size_t i = 0; i < it->second.size(); ++i)
			{
				const Action &action = it->second[i];
				switch (action.verb)
				{
				case Action::Write:
				{
					std::vector<ControlValue> args;
					for (size_t n = 0; n < action.with.size(); ++n)
					{
						const Argument &a = action.with[n];
						if (a.kind == Argument::Constant)
							args.push_back(ControlValue(a.constant));
						else if (a.kind == Argument::Attribute)
							args.push_back(ControlValue(sink.Attribute(a.control, a.attribute)));
						else
						{
							const Arguments::const_iterator named = arguments.find(a.name);
							args.push_back(a.name == "value" ? value :
								       named == arguments.end() ? ControlValue(0.0) : named->second);
						}
					}
					sink.Write(action.properties[0], action.hasValue ? action.value : value, args);
					break;
				}
				case Action::Delta:
					for (size_t p = 0; p < action.properties.size(); ++p)
						value = ControlValue(sink.Delta(action.properties[p], action.amount, action.factor,
										action.byFactor));
					break;
				case Action::Set:
					sink.Set(action.control, action.attribute, value);
					break;
				case Action::Clear:
					for (size_t c = 0; c < action.controls.size(); ++c)
						sink.Clear(action.controls[c]);
					break;
				}
			}
		}
	}
}
