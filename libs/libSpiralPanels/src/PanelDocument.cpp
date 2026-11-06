// SPDX-License-Identifier: GPL-2.0-or-later
#include "PanelDocument.h"
#include "JSON.h"
#include <cstdio>
#include <fstream>
#include <sstream>

namespace Spiral
{
	namespace Panels
	{
		Document::Document():
			m_Panel(NULL)
		{
		}

		Document::~Document()
		{
			Clear();
		}

		void Document::Clear()
		{
			for (std::map<std::string, Spumoni::JSON *>::iterator i = m_Fields.begin(); i != m_Fields.end(); ++i)
				delete i->second;

			m_Fields.clear();
			delete m_Panel;
			m_Panel = NULL;
		}

		bool Document::Load(const std::string &text, std::string &error)
		{
			std::string problem;
			Spumoni::JSONOwner root(Spumoni::ParseJSONText(text, &problem));

			if (!root.get())
			{
				error = problem.empty() ? std::string("not JSON") : problem;

				return false;
			}

			return Load(*root, error);
		}

		bool Document::Load(const Spumoni::JSON &root, std::string &error)
		{
			if (root.GetType() != Spumoni::JSON::Object)
			{
				error = "a panel file is a JSON object";

				return false;
			}

			Clear();

			const std::vector<std::string> keys = root.Keys();

			for (size_t i = 0; i < keys.size(); ++i)
			{
				const Spumoni::JSON *value = root.Get(keys[i]);

				if (keys[i] == "panel" && value->GetType() == Spumoni::JSON::Object)
					m_Panel = Node::FromJSON(*value);
				else
					m_Fields[keys[i]] = value->Duplicate();
			}

			return true;
		}

		bool Document::LoadFile(const std::string &path, std::string &error)
		{
			std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);

			if (!in)
			{
				error = "cannot read " + path;

				return false;
			}

			std::ostringstream text;
			text << in.rdbuf();

			if (Load(text.str(), error))
				return true;

			error = path + ": " + error;

			return false;
		}

		Spumoni::JSON *Document::ToJSON() const
		{
			Spumoni::JSON *root = Spumoni::JSON::MakeObject();

			for (std::map<std::string, Spumoni::JSON *>::const_iterator i = m_Fields.begin(); i != m_Fields.end(); ++i)
				root->SetOwned(i->first, i->second->Duplicate());

			if (m_Panel)
				root->SetOwned("panel", m_Panel->ToJSON());

			return root;
		}

		bool Document::Save(std::string &text, std::string &error, bool pretty) const
		{
			Spumoni::JSONOwner root(ToJSON());
			const std::string written = root->Stringify(pretty);
			std::string problem;
			Spumoni::JSONOwner check(Spumoni::ParseJSONText(written, &problem));

			if (!check.get())
			{
				error = "the panel does not write as JSON: " + problem;

				return false;
			}

			text = written;

			return true;
		}

		bool Document::SaveFile(const std::string &path, std::string &error) const
		{
			std::string text;

			if (!Save(text, error, true))
				return false;

			std::ofstream out(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
			out << text << "\n";

			if (!out)
			{
				error = "cannot write " + path;

				return false;
			}

			return true;
		}

		Document *Document::Clone() const
		{
			Spumoni::JSONOwner root(ToJSON());
			Document *copy = new Document();
			std::string error;
			copy->Load(*root, error);

			return copy;
		}

		const Spumoni::JSON *Document::Field(const std::string &name) const
		{
			std::map<std::string, Spumoni::JSON *>::const_iterator found = m_Fields.find(name);

			return found == m_Fields.end() ? NULL : found->second;
		}

		std::vector<std::string> Document::FieldNames() const
		{
			std::vector<std::string> names;

			for (std::map<std::string, Spumoni::JSON *>::const_iterator i = m_Fields.begin(); i != m_Fields.end(); ++i)
				names.push_back(i->first);

			return names;
		}

		void Document::SetField(const std::string &name, Spumoni::JSON *owned)
		{
			RemoveField(name);

			if (name == "panel" && owned->GetType() == Spumoni::JSON::Object)
			{
				m_Panel = Node::FromJSON(*owned);
				delete owned;

				return;
			}

			m_Fields[name] = owned;
		}

		void Document::RemoveField(const std::string &name)
		{
			if (name == "panel")
			{
				delete m_Panel;
				m_Panel = NULL;
			}

			std::map<std::string, Spumoni::JSON *>::iterator found = m_Fields.find(name);

			if (found == m_Fields.end())
				return;

			delete found->second;
			m_Fields.erase(found);
		}

		bool Document::PluginId(long &out) const
		{
			const Spumoni::JSON *value = Field("plugin_id");

			return value && value->Integer(out);
		}

		bool Document::SchemaVersion(long &out) const
		{
			const Spumoni::JSON *value = Field("schema_version");

			return value && value->Integer(out);
		}

		bool Document::DeclaresContracts() const
		{
			return Field("control_contracts") != NULL;
		}

		bool Document::Contracts(std::map<std::string, long> &out) const
		{
			const Spumoni::JSON *declared = Field("control_contracts");
			out.clear();

			if (!declared || declared->GetType() != Spumoni::JSON::Object)
				return false;

			const std::vector<std::string> names = declared->Keys();

			for (size_t i = 0; i < names.size(); ++i)
			{
				long version;

				if (!declared->Get(names[i])->Integer(version))
				{
					out.clear();

					return false;
				}

				out[names[i]] = version;
			}

			return true;
		}

		void Document::SetContracts(const std::map<std::string, long> &versions)
		{
			Spumoni::JSON *declared = Spumoni::JSON::MakeObject();

			for (std::map<std::string, long>::const_iterator i = versions.begin(); i != versions.end(); ++i)
			{
				char text[32];
				snprintf(text, sizeof text, "%ld", i->second);
				declared->SetOwned(i->first, Spumoni::JSON::MakeNumber(text));
			}

			SetField("control_contracts", declared);
		}
	}
}
