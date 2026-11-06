// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_CONTRACTS_H
#define SPIRAL_PANELS_CONTRACTS_H

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Spumoni
{
	class JSON;
}

namespace Spiral
{
	namespace Panels
	{
		class Document;
		class Node;

		struct Contract
		{
			std::string Qualified;
			bool HasExtends;
			std::string Extends;
			bool Abstract;
			std::map<std::string, std::vector<std::string> > Attributes;
			std::vector<std::string> Required;
			Contract(): HasExtends(false), Abstract(false) {}
		};

		typedef std::map<std::string, Contract> ContractTable;

		extern const char *const RootNamespace;

		class Registry
		{
		public:
			bool Add(const Spumoni::JSON &document, std::string &error);
			bool AddFile(const std::string &path, std::string &error);
			bool LoadDirectory(const std::string &directory, std::string &error);

			const ContractTable *Table(const std::string &space, long version) const;
			std::vector<long> Versions(const std::string &space) const;
			std::vector<std::string> Namespaces() const;
			std::map<std::string, long> Oldest() const;
			std::map<std::string, long> Newest() const;

		private:
			std::map<std::pair<std::string, long>, ContractTable> m_Tables;
		};

		enum Status
		{
			Valid,
			Extended,
			Invalid,
			Unresolved
		};

		const char *StatusName(Status status);

		struct NodeReport
		{
			std::string Path;
			const Node *Target;
			bool HasType;
			std::string Type;
			std::string Contract;
			Status Result;
			std::vector<std::string> Problems;
			std::vector<std::string> Extensions;
			NodeReport(): Target(NULL), HasType(false), Result(Unresolved) {}
		};

		struct Report
		{
			std::map<std::string, long> Contracts;
			bool Assumed;
			std::vector<std::string> Problems;
			std::vector<NodeReport> Nodes;
			Report(): Assumed(false) {}

			size_t Count(Status status) const;
			bool Ok() const;
			const NodeReport *At(const std::string &path) const;
		};

		Report Resolve(const Document &document, const Registry &registry,
				const std::map<std::string, long> &assume = std::map<std::string, long>());

		bool IsText(const Spumoni::JSON *value);
		bool IsBinding(const Spumoni::JSON *value);
		bool IsHandlers(const Spumoni::JSON *value);
		bool IsKind(const std::string &kind, const Spumoni::JSON *value);
		bool KnownKind(const std::string &kind);
		bool Placeholder(const Spumoni::JSON *value, std::string &name);
		std::set<std::string> TemplateVariables(const Node &repeat);
	}
}

#endif
