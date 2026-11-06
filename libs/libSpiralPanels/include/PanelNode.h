// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_NODE_H
#define SPIRAL_PANELS_NODE_H

#include "ValueRule.h"
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
		struct Bounds
		{
			long X, Y, W, H;
			Bounds(): X(0), Y(0), W(0), H(0) {}
			Bounds(long x, long y, long w, long h): X(x), Y(y), W(w), H(h) {}
		};

		class Node;

		class Page
		{
		public:
			Page();
			~Page();

			static Page *FromJSON(const Spumoni::JSON &value);
			Spumoni::JSON *ToJSON() const;
			Page *Clone() const;

			bool IsObject() const { return m_Raw == NULL; }
			const Spumoni::JSON *Field(const std::string &name) const;
			bool Text(const std::string &name, Data::ValueRule::Text &out) const;
			bool HasChildren() const { return m_HasChildren; }
			std::vector<Node *> &Children() { return m_Children; }
			const std::vector<Node *> &Children() const { return m_Children; }

		private:
			Page(const Page &);
			Page &operator=(const Page &);

			Spumoni::JSON *m_Raw;
			std::map<std::string, Spumoni::JSON *> m_Fields;
			bool m_HasChildren;
			std::vector<Node *> m_Children;
		};

		class Node
		{
		public:
			Node();
			~Node();

			static Node *FromJSON(const Spumoni::JSON &value);
			Spumoni::JSON *ToJSON() const;
			Node *Clone() const;

			bool IsObject() const { return m_Raw == NULL; }

			bool HasType() const { return m_HasType; }
			const std::string &Type() const { return m_Type; }
			std::string Namespace() const;
			std::string ShortType() const;
			void SetType(const std::string &type);

			bool HasName() const { return m_HasName; }
			const std::string &Name() const { return m_Name; }
			void SetName(const std::string &name);

			bool HasBounds() const { return m_HasBounds; }
			const Bounds &GetBounds() const { return m_Bounds; }
			void SetBounds(const Bounds &bounds);

			bool Has(const std::string &name) const;
			const Spumoni::JSON *Attribute(const std::string &name) const;
			Spumoni::JSON *Attribute(const std::string &name);
			std::vector<std::string> AttributeNames() const;
			void SetAttribute(const std::string &name, Spumoni::JSON *owned);
			Spumoni::JSON *TakeAttribute(const std::string &name);
			void RemoveAttribute(const std::string &name);

			bool String(const std::string &name, std::string &out) const;
			bool Integer(const std::string &name, long &out) const;
			bool Number(const std::string &name, double &out) const;
			bool Boolean(const std::string &name, bool &out) const;
			bool Text(const std::string &name, Data::ValueRule::Text &out) const;

			bool HasChildren() const { return m_HasChildren; }
			std::vector<Node *> &Children() { return m_Children; }
			const std::vector<Node *> &Children() const { return m_Children; }
			void SetChildren(const std::vector<Node *> &owned);

			bool HasPages() const { return m_HasPages; }
			std::vector<Page *> &Pages() { return m_Pages; }
			const std::vector<Page *> &Pages() const { return m_Pages; }

		private:
			Node(const Node &);
			Node &operator=(const Node &);
			void ClearChildren();

			Spumoni::JSON *m_Raw;
			bool m_HasType, m_HasName, m_HasBounds;
			std::string m_Type, m_Name;
			Bounds m_Bounds;
			std::map<std::string, Spumoni::JSON *> m_Attributes;
			bool m_HasChildren;
			std::vector<Node *> m_Children;
			bool m_HasPages;
			std::vector<Page *> m_Pages;
		};
	}
}

#endif
