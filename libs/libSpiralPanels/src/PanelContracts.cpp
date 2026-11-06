// SPDX-License-Identifier: GPL-2.0-or-later
#include "PanelContracts.h"
#include "PanelDocument.h"
#include "PanelNode.h"
#include "JSON.h"
#include <algorithm>
#include <cstdio>
#include <dirent.h>
#include <iterator>
#include <sstream>

namespace Spiral
{
	namespace Panels
	{
		typedef Spumoni::JSON JSON;

		extern const char *const RootNamespace = "Controls";

		static bool IsString(const JSON *value)
		{
			return value && value->GetType() == JSON::String;
		}

		static std::string Join(const std::vector<std::string> &parts, const std::string &separator)
		{
			std::string out;

			for (size_t i = 0; i < parts.size(); ++i)
			{
				if (i)
					out += separator;

				out += parts[i];
			}

			return out;
		}

		static std::string Number(long value)
		{
			char text[32];
			snprintf(text, sizeof text, "%ld", value);

			return text;
		}

		static const char *KindName(const JSON *value)
		{
			switch (value->GetType())
			{
				case JSON::Null:
					return "null";
				case JSON::Boolean:
					return "boolean";
				case JSON::Number:
					return "number";
				case JSON::String:
					return "string";
				case JSON::Array:
					return "array";
				case JSON::Object:
					return "object";
			}

			return "value";
		}

		bool IsText(const JSON *value)
		{
			if (IsString(value))
				return true;

			if (!value || value->GetType() != JSON::Object || !IsString(value->Get("default")))
				return false;

			const std::vector<std::string> keys = value->Keys();

			for (size_t i = 0; i < keys.size(); ++i)
			{
				if (keys[i] != "default" && keys[i] != "key" && keys[i] != "translations")
					return false;
			}

			return true;
		}

		bool IsBinding(const JSON *value)
		{
			if (!value || value->GetType() != JSON::Object || value->Size() == 0)
				return false;

			const std::vector<std::string> keys = value->Keys();

			if (keys.size() == 1 && (keys[0] == "any" || keys[0] == "all"))
			{
				const JSON *cases = value->Get(keys[0]);

				if (cases->GetType() != JSON::Array || cases->Size() == 0)
					return false;

				for (size_t i = 0; i < cases->Size(); ++i)
				{
					if (!IsBinding(cases->At(i)))
						return false;
				}

				return true;
			}

			if (keys.size() == 1 && keys[0] == "not")
				return IsBinding(value->Get("not"));

			if (IsString(value->Get("property")))
			{
				std::vector<std::string> extra;

				for (size_t i = 0; i < keys.size(); ++i)
				{
					if (keys[i] != "property")
						extra.push_back(keys[i]);
				}

				if (extra.empty())
					return true;

				if (extra.size() == 2 && value->Get("on") && value->Get("off"))
					return IsText(value->Get("on")) && IsText(value->Get("off"));

				if (extra.size() != 1)
					return false;

				if (extra[0] == "one_of")
					return value->Get("one_of")->GetType() == JSON::Array;

				return extra[0] == "equals" || extra[0] == "not_equals";
			}

			if (keys.size() == 2 && value->Get("control") && value->Get("attribute"))
				return IsString(value->Get("control")) && IsString(value->Get("attribute"));

			return false;
		}

		static bool VerbAccepts(const std::string &verb, const JSON *argument)
		{
			if (verb == "write" || verb == "set")
				return IsString(argument);

			if (verb == "delta")
				return IsString(argument) || argument->GetType() == JSON::Array;

			if (verb == "clear")
				return argument->GetType() == JSON::Array;

			return false;
		}

		bool IsHandlers(const JSON *value)
		{
			if (!value || value->GetType() != JSON::Object || value->Size() == 0)
				return false;

			const std::vector<std::string> events = value->Keys();

			for (size_t e = 0; e < events.size(); ++e)
			{
				const JSON *actions = value->Get(events[e]);

				if (actions->GetType() != JSON::Array || actions->Size() == 0)
					return false;

				for (size_t a = 0; a < actions->Size(); ++a)
				{
					const JSON *action = actions->At(a);

					if (action->GetType() != JSON::Object)
						return false;

					static const char *verbs[] = {"write", "delta", "set", "clear"};
					int found = 0;
					std::string verb;

					for (size_t v = 0; v < 4; ++v)
					{
						if (action->Get(verbs[v]))
						{
							++found;
							verb = verbs[v];
						}
					}

					if (found != 1 || !VerbAccepts(verb, action->Get(verb)))
						return false;
				}
			}

			return true;
		}

		bool KnownKind(const std::string &kind)
		{
			static const char *kinds[] = {"string", "integer", "number", "boolean", "object", "array", "null",
					"text", "binding", "handlers", "nodes", "template", "pages"};

			for (size_t i = 0; i < sizeof kinds / sizeof kinds[0]; ++i)
			{
				if (kind == kinds[i])
					return true;
			}

			return false;
		}

		bool IsKind(const std::string &kind, const JSON *value)
		{
			long integer;

			if (kind == "string")
				return value->GetType() == JSON::String;

			if (kind == "integer")
				return value->Integer(integer);

			if (kind == "number")
				return value->GetType() == JSON::Number;

			if (kind == "boolean")
				return value->GetType() == JSON::Boolean;

			if (kind == "object")
				return value->GetType() == JSON::Object;

			if (kind == "array" || kind == "nodes" || kind == "template" || kind == "pages")
				return value->GetType() == JSON::Array;

			if (kind == "null")
				return value->GetType() == JSON::Null;

			if (kind == "text")
				return IsText(value);

			if (kind == "binding")
				return IsBinding(value);

			if (kind == "handlers")
				return IsHandlers(value);

			return false;
		}

		static bool NameStart(char c)
		{
			return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
		}

		static bool NamePart(char c)
		{
			return NameStart(c) || (c >= '0' && c <= '9');
		}

		bool Placeholder(const JSON *value, std::string &name)
		{
			if (!IsString(value))
				return false;

			const std::string &text = value->Text();

			if (text.size() < 3 || text[0] != '{' || text[text.size() - 1] != '}' || !NameStart(text[1]))
				return false;

			for (size_t i = 2; i + 1 < text.size(); ++i)
			{
				if (!NamePart(text[i]))
					return false;
			}

			name = text.substr(1, text.size() - 2);

			return true;
		}

		std::set<std::string> TemplateVariables(const Node &repeat)
		{
			std::set<std::string> names;
			names.insert("index");
			names.insert("index0");
			const JSON *items = repeat.Attribute("items");

			if (!items || items->GetType() != JSON::Array || items->Size() == 0)
				return names;

			for (size_t i = 0; i < items->Size(); ++i)
			{
				if (items->At(i)->GetType() != JSON::Object)
					return names;
			}

			std::vector<std::string> shared = items->At(0)->Keys();

			for (size_t i = 1; i < items->Size(); ++i)
			{
				const std::vector<std::string> keys = items->At(i)->Keys();
				std::vector<std::string> both;
				std::set_intersection(shared.begin(), shared.end(), keys.begin(), keys.end(), std::back_inserter(both));
				shared = both;
			}

			names.insert(shared.begin(), shared.end());

			return names;
		}

		static std::vector<std::string> Strings(const JSON *array)
		{
			std::vector<std::string> out;

			if (!array || array->GetType() != JSON::Array)
				return out;

			for (size_t i = 0; i < array->Size(); ++i)
			{
				if (IsString(array->At(i)))
					out.push_back(array->At(i)->Text());
			}

			return out;
		}

		bool Registry::Add(const JSON &document, std::string &error)
		{
			const JSON *space = document.Get("namespace");
			const JSON *version = document.Get("version");
			const JSON *controls = document.Get("controls");
			long number;

			if (document.GetType() != JSON::Object || !IsString(space) || !version || !controls
			    || controls->GetType() != JSON::Object)
			{
				error = "contract document needs namespace, version, controls";

				return false;
			}

			if (!version->Integer(number))
			{
				error = space->Text() + ": version must be an integer";

				return false;
			}

			ContractTable table;
			const std::vector<std::string> names = controls->Keys();

			for (size_t i = 0; i < names.size(); ++i)
			{
				const JSON *spec = controls->Get(names[i]);
				Contract contract;
				contract.Qualified = space->Text() + "::" + names[i];

				if (spec->GetType() != JSON::Object)
				{
					error = contract.Qualified + ": a contract is an object";

					return false;
				}

				if (IsString(spec->Get("extends")))
				{
					contract.HasExtends = true;
					contract.Extends = spec->Get("extends")->Text();
				}

				contract.Abstract = spec->Get("abstract") && spec->Get("abstract")->GetType() == JSON::Boolean
						&& spec->Get("abstract")->AsBool();
				contract.Required = Strings(spec->Get("required"));
				const JSON *attributes = spec->Get("attributes");

				if (attributes && attributes->GetType() == JSON::Object)
				{
					const std::vector<std::string> keys = attributes->Keys();

					for (size_t a = 0; a < keys.size(); ++a)
					{
						const std::vector<std::string> kinds = Strings(attributes->Get(keys[a]));
						std::vector<std::string> unknown;

						for (size_t k = 0; k < kinds.size(); ++k)
						{
							if (!KnownKind(kinds[k]))
								unknown.push_back(kinds[k]);
						}

						if (kinds.empty() || !unknown.empty())
						{
							error = contract.Qualified + "." + keys[a] + ": unknown kinds " + Join(unknown, ", ");

							return false;
						}

						contract.Attributes[keys[a]] = kinds;
					}
				}

				table[names[i]] = contract;
			}

			m_Tables[std::make_pair(space->Text(), number)] = table;

			return true;
		}

		bool Registry::AddFile(const std::string &path, std::string &error)
		{
			std::string problem;
			Spumoni::JSONOwner document(Spumoni::ParseJSON(path.c_str(), &problem));

			if (!document.get())
			{
				error = path + ": " + problem;

				return false;
			}

			if (Add(*document, error))
				return true;

			error = path + ": " + error;

			return false;
		}

		bool Registry::LoadDirectory(const std::string &directory, std::string &error)
		{
			DIR *dir = opendir(directory.c_str());

			if (!dir)
			{
				error = "cannot read " + directory;

				return false;
			}

			std::vector<std::string> names;

			while (struct dirent *entry = readdir(dir))
			{
				const std::string name = entry->d_name;

				if (name.size() > 5 && name.compare(name.size() - 5, 5, ".json") == 0)
					names.push_back(name);
			}

			closedir(dir);
			std::sort(names.begin(), names.end());

			if (names.empty())
			{
				error = "no contract documents in " + directory;

				return false;
			}

			for (size_t i = 0; i < names.size(); ++i)
			{
				if (!AddFile(directory + "/" + names[i], error))
					return false;
			}

			return true;
		}

		const ContractTable *Registry::Table(const std::string &space, long version) const
		{
			std::map<std::pair<std::string, long>, ContractTable>::const_iterator found =
					m_Tables.find(std::make_pair(space, version));

			return found == m_Tables.end() ? NULL : &found->second;
		}

		std::vector<long> Registry::Versions(const std::string &space) const
		{
			std::vector<long> versions;

			for (std::map<std::pair<std::string, long>, ContractTable>::const_iterator i = m_Tables.begin();
					i != m_Tables.end(); ++i)
			{
				if (i->first.first == space)
					versions.push_back(i->first.second);
			}

			return versions;
		}

		std::vector<std::string> Registry::Namespaces() const
		{
			std::vector<std::string> spaces;

			for (std::map<std::pair<std::string, long>, ContractTable>::const_iterator i = m_Tables.begin();
					i != m_Tables.end(); ++i)
			{
				if (spaces.empty() || spaces.back() != i->first.first)
					spaces.push_back(i->first.first);
			}

			return spaces;
		}

		std::map<std::string, long> Registry::Oldest() const
		{
			std::map<std::string, long> versions;
			const std::vector<std::string> spaces = Namespaces();

			for (size_t i = 0; i < spaces.size(); ++i)
				versions[spaces[i]] = Versions(spaces[i]).front();

			return versions;
		}

		std::map<std::string, long> Registry::Newest() const
		{
			std::map<std::string, long> versions;
			const std::vector<std::string> spaces = Namespaces();

			for (size_t i = 0; i < spaces.size(); ++i)
				versions[spaces[i]] = Versions(spaces[i]).back();

			return versions;
		}

		const char *StatusName(Status status)
		{
			switch (status)
			{
				case Valid:
					return "valid";
				case Extended:
					return "extended";
				case Invalid:
					return "invalid";
				case Unresolved:
					return "unresolved";
			}

			return "unresolved";
		}

		size_t Report::Count(Status status) const
		{
			size_t count = 0;

			for (size_t i = 0; i < Nodes.size(); ++i)
			{
				if (Nodes[i].Result == status)
					++count;
			}

			return count;
		}

		bool Report::Ok() const
		{
			return Problems.empty() && Count(Valid) == Nodes.size();
		}

		const NodeReport *Report::At(const std::string &path) const
		{
			for (size_t i = 0; i < Nodes.size(); ++i)
			{
				if (Nodes[i].Path == path)
					return &Nodes[i];
			}

			return NULL;
		}

		namespace
		{
			typedef std::map<std::string, std::vector<std::string> > Attributes;

			class Resolver
			{
			public:
				Resolver(const Registry &registry, Report &report):
					m_Report(report)
				{
					for (std::map<std::string, long>::const_iterator i = report.Contracts.begin();
							i != report.Contracts.end(); ++i)
					{
						const ContractTable *table = registry.Table(i->first, i->second);

						if (table)
						{
							m_Tables[i->first] = table;

							continue;
						}

						std::vector<std::string> have;
						const std::vector<long> versions = registry.Versions(i->first);

						for (size_t v = 0; v < versions.size(); ++v)
							have.push_back(Number(versions[v]));

						report.Problems.push_back(i->first + " " + Number(i->second) + " is not available (have ["
								+ Join(have, ", ") + "])");
					}
				}

				const Contract *Find(const std::string &type, std::string &problem) const
				{
					const std::string::size_type at = type.find("::");

					if (at != std::string::npos)
					{
						const std::string space = type.substr(0, at);
						const std::string name = type.substr(at + 2);
						std::map<std::string, const ContractTable *>::const_iterator table = m_Tables.find(space);

						if (table == m_Tables.end())
						{
							problem = type + ": namespace " + space + " is not declared or not available";

							return NULL;
						}

						ContractTable::const_iterator found = table->second->find(name);

						if (found == table->second->end())
						{
							problem = type + ": not in the selected " + space + " contracts";

							return NULL;
						}

						return &found->second;
					}

					std::map<std::string, const ContractTable *>::const_iterator root = m_Tables.find(RootNamespace);

					if (root == m_Tables.end())
					{
						problem = type + ": the " + RootNamespace + " controls are not declared or not available";

						return NULL;
					}

					ContractTable::const_iterator found = root->second->find(type);

					if (found == root->second->end())
					{
						problem = type + ": not a " + RootNamespace + " control; a custom control is written with its namespace";

						return NULL;
					}

					return &found->second;
				}

				bool Flatten(const Contract *contract, Attributes &attributes, std::set<std::string> &required,
						std::string &problem) const
				{
					std::vector<const Contract *> chain;
					std::set<std::string> seen;

					while (contract)
					{
						if (seen.count(contract->Qualified))
						{
							problem = contract->Qualified + ": extends cycle";

							return false;
						}

						seen.insert(contract->Qualified);
						chain.push_back(contract);

						if (!contract->HasExtends)
							break;

						std::string missing;
						const Contract *base = Find(contract->Extends, missing);

						if (!base)
						{
							problem = contract->Qualified + ": base " + missing;

							return false;
						}

						contract = base;
					}

					for (size_t i = chain.size(); i-- > 0;)
					{
						for (Attributes::const_iterator a = chain[i]->Attributes.begin(); a != chain[i]->Attributes.end(); ++a)
							attributes[a->first] = a->second;

						required.insert(chain[i]->Required.begin(), chain[i]->Required.end());
					}

					return true;
				}

				void Walk(const Node &node, const std::string &path, const std::set<std::string> *variables)
				{
					NodeReport entry;
					entry.Path = path;
					entry.Target = &node;

					if (!node.IsObject())
					{
						entry.Problems.push_back("not an object");
						m_Report.Nodes.push_back(entry);

						return;
					}

					if (!node.HasType())
					{
						entry.Problems.push_back("type is missing or not a string");
						m_Report.Nodes.push_back(entry);

						return;
					}

					entry.HasType = true;
					entry.Type = node.Type();
					std::string problem;
					const Contract *contract = Find(node.Type(), problem);

					if (!contract)
					{
						entry.Problems.push_back(problem);
						m_Report.Nodes.push_back(entry);

						return;
					}

					if (contract->Abstract)
					{
						entry.Contract = contract->Qualified;
						entry.Result = Invalid;
						entry.Problems.push_back(contract->Qualified + " is abstract");
						m_Report.Nodes.push_back(entry);

						return;
					}

					Attributes attributes;
					std::set<std::string> required;

					if (!Flatten(contract, attributes, required, problem))
					{
						entry.Problems.push_back(problem);
						m_Report.Nodes.push_back(entry);

						return;
					}

					entry.Contract = contract->Qualified;
					Spumoni::JSONOwner object(node.ToJSON());
					const std::vector<std::string> keys = object->Keys();

					for (size_t i = 0; i < keys.size(); ++i)
					{
						if (keys[i] == "type")
							continue;

						const JSON *value = object->Get(keys[i]);
						Attributes::const_iterator kinds = attributes.find(keys[i]);

						if (kinds == attributes.end())
						{
							entry.Extensions.push_back(keys[i]);

							continue;
						}

						bool matched = false;

						for (size_t k = 0; k < kinds->second.size() && !matched; ++k)
							matched = IsKind(kinds->second[k], value);

						if (matched)
							continue;

						std::string name;

						if (variables && Placeholder(value, name))
						{
							if (!variables->count(name))
								entry.Problems.push_back(keys[i] + ": placeholder " + value->Text()
										+ " is not supplied by every item");

							continue;
						}

						entry.Problems.push_back(keys[i] + ": expected " + Join(kinds->second, "/") + ", got "
								+ KindName(value));
					}

					for (std::set<std::string>::const_iterator i = required.begin(); i != required.end(); ++i)
					{
						if (!object->Get(*i))
							entry.Problems.push_back(*i + ": required");
					}

					entry.Result = !entry.Problems.empty() ? Invalid : !entry.Extensions.empty() ? Extended : Valid;
					m_Report.Nodes.push_back(entry);

					for (Attributes::const_iterator a = attributes.begin(); a != attributes.end(); ++a)
					{
						const std::vector<std::string> &kinds = a->second;
						const bool nodes = std::find(kinds.begin(), kinds.end(), "nodes") != kinds.end();
						const bool templated = std::find(kinds.begin(), kinds.end(), "template") != kinds.end();
						const bool pages = std::find(kinds.begin(), kinds.end(), "pages") != kinds.end();

						if (a->first == "children" && node.HasChildren() && (nodes || templated))
						{
							std::set<std::string> scope;
							const std::set<std::string> *inner = variables;

							if (templated)
							{
								scope = TemplateVariables(node);
								inner = &scope;
							}

							for (size_t i = 0; i < node.Children().size(); ++i)
								Walk(*node.Children()[i], path + "/children/" + Number((long)i), inner);
						}
						else if (a->first == "pages" && node.HasPages() && pages)
						{
							for (size_t p = 0; p < node.Pages().size(); ++p)
							{
								const Page &page = *node.Pages()[p];

								if (!page.HasChildren())
									continue;

								for (size_t i = 0; i < page.Children().size(); ++i)
									Walk(*page.Children()[i], path + "/pages/" + Number((long)p) + "/children/"
											+ Number((long)i), variables);
							}
						}
					}
				}

			private:
				Report &m_Report;
				std::map<std::string, const ContractTable *> m_Tables;
			};
		}

		Report Resolve(const Document &document, const Registry &registry, const std::map<std::string, long> &assume)
		{
			Report report;

			if (!document.DeclaresContracts())
			{
				report.Contracts = assume;
				report.Assumed = true;
			}
			else if (!document.Contracts(report.Contracts))
				report.Problems.push_back("control_contracts must map namespace names to integer versions");

			Resolver resolver(registry, report);
			const Node *panel = document.Panel();

			if (!panel || !panel->HasChildren())
			{
				report.Problems.push_back("panel/children is not an array");

				return report;
			}

			for (size_t i = 0; i < panel->Children().size(); ++i)
				resolver.Walk(*panel->Children()[i], "panel/children/" + Number((long)i), NULL);

			return report;
		}
	}
}
