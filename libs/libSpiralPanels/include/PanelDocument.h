// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_DOCUMENT_H
#define SPIRAL_PANELS_DOCUMENT_H

#include "PanelNode.h"
#include <map>
#include <string>
#include <vector>

namespace Spumoni
{
	class JSON;
}

namespace Spiral
{
	namespace Panels
	{
		class Document
		{
		public:
			Document();
			~Document();

			bool Load(const std::string &text, std::string &error);
			bool Load(const Spumoni::JSON &root, std::string &error);
			bool LoadFile(const std::string &path, std::string &error);
			bool Save(std::string &text, std::string &error, bool pretty = false) const;
			bool SaveFile(const std::string &path, std::string &error) const;
			Spumoni::JSON *ToJSON() const;
			Document *Clone() const;
			void Clear();

			const Spumoni::JSON *Field(const std::string &name) const;
			std::vector<std::string> FieldNames() const;
			void SetField(const std::string &name, Spumoni::JSON *owned);
			void RemoveField(const std::string &name);

			bool PluginId(long &out) const;
			bool SchemaVersion(long &out) const;

			bool DeclaresContracts() const;
			bool Contracts(std::map<std::string, long> &out) const;
			void SetContracts(const std::map<std::string, long> &versions);

			const Spumoni::JSON *Properties() const { return Field("properties"); }
			const Spumoni::JSON *State() const { return Field("state"); }

			bool HasPanel() const { return m_Panel != NULL; }
			Node *Panel() { return m_Panel; }
			const Node *Panel() const { return m_Panel; }

		private:
			Document(const Document &);
			Document &operator=(const Document &);

			std::map<std::string, Spumoni::JSON *> m_Fields;
			Node *m_Panel;
		};
	}
}

#endif
