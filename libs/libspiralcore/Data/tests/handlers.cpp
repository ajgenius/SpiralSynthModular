// SPDX-License-Identifier: GPL-2.0-or-later
#include "Handlers.h"
#include "JSON.h"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

using namespace Spiral::Data;

namespace
{
	std::string Spell(const ControlValue &value)
	{
		char text[64];
		if (value.type == ControlValue::String)
			return value.text;
		if (value.type == ControlValue::Boolean)
			return value.boolean ? "true" : "false";
		std::snprintf(text, sizeof(text), "%g", value.number);
		return text;
	}

	std::string Spell(double number)
	{
		char text[64];
		std::snprintf(text, sizeof(text), "%g", number);
		return text;
	}

	struct FixtureScope: Handlers::Scope
	{
		std::map<std::string, double> Fields;
		std::map<std::string, std::map<std::string, double> > Controls;
		std::map<std::string, std::vector<std::string> > Events;

		bool HasField(const std::string &name) const { return Fields.count(name) != 0; }
		bool HasControl(const std::string &name) const { return Controls.count(name) != 0; }
		bool HasAttribute(const std::string &control, const std::string &attribute) const
		{
			std::map<std::string, std::map<std::string, double> >::const_iterator c = Controls.find(control);
			return c != Controls.end() && c->second.count(attribute) != 0;
		}
		bool CanSetAttribute(const std::string &control, const std::string &attribute) const
		{
			return HasAttribute(control, attribute);
		}
		bool Event(const std::string &name, std::vector<std::string> &arguments) const
		{
			std::map<std::string, std::vector<std::string> >::const_iterator e = Events.find(name);
			if (e == Events.end())
				return false;
			arguments = e->second;
			return true;
		}
	};

	struct RecordingSink: Handlers::Sink
	{
		const FixtureScope &scope;
		std::vector<std::string> calls;
		explicit RecordingSink(const FixtureScope &s): scope(s) {}

		void Write(const std::string &property, const ControlValue &value, const std::vector<ControlValue> &arguments)
		{
			std::string line = "write " + property + " " + Spell(value) + " [";
			for (size_t i = 0; i < arguments.size(); ++i)
				line += (i ? "," : "") + Spell(arguments[i]);
			calls.push_back(line + "]");
		}
		double Delta(const std::string &property, double amount, double factor, bool byFactor)
		{
			calls.push_back("delta " + property + (byFactor ? " factor " + Spell(factor) : " amount " + Spell(amount)));
			double &held = const_cast<FixtureScope &>(scope).Fields[property];
			return held = byFactor ? held * factor : held + amount;
		}
		void Set(const std::string &control, const std::string &attribute, const ControlValue &value)
		{
			calls.push_back("set " + control + " " + attribute + " " + Spell(value));
		}
		void Clear(const std::string &control) { calls.push_back("clear " + control); }
		double Attribute(const std::string &control, const std::string &attribute) const
		{
			return scope.Controls.find(control)->second.find(attribute)->second;
		}
	};

	ControlValue ValueOf(const Spumoni::JSON *node)
	{
		if (node->GetType() == Spumoni::JSON::String)
			return ControlValue(node->Text());
		if (node->GetType() == Spumoni::JSON::Boolean)
			return ControlValue(node->AsBool());
		return ControlValue(std::atof(node->Text().c_str()));
	}

	bool Read(const Spumoni::JSON *entry, const FixtureScope &scope, Handlers &out, std::string &problem)
	{
		if (entry->Get("writes"))
		{
			Handlers::Actions actions;
			const std::string event = entry->Get("event")->Text();
			return Handlers::ParseWrites(entry->Get("writes"), scope, event, actions, problem) &&
				out.Add(event, actions, scope, problem);
		}
		return Handlers::Parse(entry->Get("on"), scope, out, problem);
	}
}

int main(int argc, char **argv)
{
	std::string path = argc > 1 ? argv[1] : std::string(getenv("SRCDIR") ? getenv("SRCDIR") : ".") + "/Data/tests/handlers.json";
	std::string error;
	Spumoni::JSON *fixture = Spumoni::ParseJSON(path.c_str(), &error);
	if (!fixture)
	{
		std::fprintf(stderr, "%s: %s\n", path.c_str(), error.c_str());
		return 2;
	}
	FixtureScope scope;
	const Spumoni::JSON *properties = fixture->Get("properties");
	std::vector<std::string> names = properties->Keys();
	for (size_t i = 0; i < names.size(); ++i)
		scope.Fields[names[i]] = std::atof(properties->Get(names[i])->Text().c_str());
	const Spumoni::JSON *controls = fixture->Get("controls");
	names = controls->Keys();
	for (size_t i = 0; i < names.size(); ++i)
	{
		scope.Controls[names[i]];
		std::vector<std::string> attributes = controls->Get(names[i])->Keys();
		for (size_t j = 0; j < attributes.size(); ++j)
			scope.Controls[names[i]][attributes[j]] =
				std::atof(controls->Get(names[i])->Get(attributes[j])->Text().c_str());
	}
	const Spumoni::JSON *events = fixture->Get("events");
	names = events->Keys();
	for (size_t i = 0; i < names.size(); ++i)
	{
		const Spumoni::JSON *arguments = events->Get(names[i]);
		std::vector<std::string> &carried = scope.Events[names[i]];
		for (size_t j = 0; j < arguments->Size(); ++j)
			carried.push_back(arguments->At(j)->Text());
	}

	int failures = 0;
	const Spumoni::JSON *cases = fixture->Get("cases");
	for (size_t i = 0; i < cases->Size(); ++i)
	{
		const Spumoni::JSON *entry = cases->At(i);
		const std::string name = entry->Get("name")->Text();
		Handlers handlers;
		std::string problem;
		if (!Read(entry, scope, handlers, problem))
		{
			std::printf("FAIL %s: refused: %s\n", name.c_str(), problem.c_str());
			++failures;
			continue;
		}
		const Spumoni::JSON *fire = entry->Get("fire");
		Handlers::Arguments arguments;
		const Spumoni::JSON *given = fire->Get("arguments");
		std::vector<std::string> keys = given->Keys();
		for (size_t k = 0; k < keys.size(); ++k)
			arguments[keys[k]] = ValueOf(given->Get(keys[k]));
		FixtureScope fresh = scope;
		RecordingSink sink(fresh);
		handlers.Fire(fire->Get("event")->Text(), arguments, sink);
		const Spumoni::JSON *expect = entry->Get("expect");
		bool same = expect->Size() == sink.calls.size();
		for (size_t k = 0; same && k < sink.calls.size(); ++k)
			same = sink.calls[k] == expect->At(k)->Text();
		if (!same)
		{
			std::printf("FAIL %s: got", name.c_str());
			for (size_t k = 0; k < sink.calls.size(); ++k)
				std::printf(" | %s", sink.calls[k].c_str());
			std::printf("\n");
			++failures;
		}
		else
			std::printf("ok   %s\n", name.c_str());
	}
	const Spumoni::JSON *invalid = fixture->Get("invalid");
	for (size_t i = 0; i < invalid->Size(); ++i)
	{
		const Spumoni::JSON *entry = invalid->At(i);
		const std::string reason = entry->Get("reason")->Text();
		Handlers handlers;
		std::string problem;
		if (Read(entry, scope, handlers, problem))
		{
			std::printf("FAIL invalid accepted: %s\n", reason.c_str());
			++failures;
		}
		else
			std::printf("ok   refused: %s (%s)\n", reason.c_str(), problem.c_str());
	}
	delete fixture;
	std::printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
