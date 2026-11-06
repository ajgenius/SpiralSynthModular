// SPDX-License-Identifier: GPL-2.0-or-later
#include "ValueRule.h"
#include "Binding.h"
#include "JSON.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

using namespace Spiral::Data;

namespace
{
	struct FixtureScope: ValueRule::Scope
	{
		std::map<std::string, double> Fields;
		std::map<std::string, std::map<std::string, double> > Attributes;

		bool HasField(const std::string &name) const { return Fields.count(name) != 0; }
		double Field(const std::string &name) const { return Fields.find(name)->second; }
		bool HasAttribute(const std::string &control, const std::string &attribute) const
		{
			std::map<std::string, std::map<std::string, double> >::const_iterator c = Attributes.find(control);
			return c != Attributes.end() && c->second.count(attribute) != 0;
		}
		double Attribute(const std::string &control, const std::string &attribute) const
		{
			return Attributes.find(control)->second.find(attribute)->second;
		}
	};

	bool Matches(const ControlValue &value, const Spumoni::JSON &expect)
	{
		switch (expect.GetType())
		{
		case Spumoni::JSON::Boolean:
			return value.type == ControlValue::Boolean && value.boolean == expect.AsBool();
		case Spumoni::JSON::String:
			return value.type == ControlValue::String && value.text == expect.Text();
		case Spumoni::JSON::Number:
			return (value.type == ControlValue::Number || value.type == ControlValue::Integer) &&
				std::fabs(value.number - std::atof(expect.Text().c_str())) < 1e-9;
		default:
			return false;
		}
	}
}

int main(int argc, char **argv)
{
	std::string path = argc > 1 ? argv[1] : std::string(getenv("SRCDIR") ? getenv("SRCDIR") : ".") + "/tests/bindings.json";
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
		std::vector<std::string> attributes = controls->Get(names[i])->Keys();
		for (size_t j = 0; j < attributes.size(); ++j)
			scope.Attributes[names[i]][attributes[j]] =
				std::atof(controls->Get(names[i])->Get(attributes[j])->Text().c_str());
	}

	int failures = 0;
	const Spumoni::JSON *cases = fixture->Get("cases");
	for (size_t i = 0; i < cases->Size(); ++i)
	{
		const Spumoni::JSON *entry = cases->At(i);
		const std::string name = entry->Get("name")->Text();
		ValueRule rule;
		std::string problem;
		if (!ValueRule::Parse(entry->Get("binding"), scope, rule, problem))
		{
			std::printf("FAIL %s: refused: %s\n", name.c_str(), problem.c_str());
			++failures;
			continue;
		}
		const ControlValue value = rule.Evaluate(scope);
		if (!Matches(value, *entry->Get("expect")))
		{
			std::printf("FAIL %s: got %s\n", name.c_str(), Binding::Format(value).c_str());
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
		ValueRule rule;
		std::string problem;
		if (ValueRule::Parse(entry->Get("binding"), scope, rule, problem))
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
