// SPDX-License-Identifier: GPL-2.0-or-later
#include "PanelNode.h"
#include "JSON.h"
#include <cstdlib>

namespace Spiral
{
	namespace Panels
	{
		typedef std::map<std::string, Spumoni::JSON *> Fields;

		static void Release(Fields &fields)
		{
			for (Fields::iterator i = fields.begin(); i != fields.end(); ++i)
				delete i->second;

			fields.clear();
		}

		static void Replace(Fields &fields, const std::string &name, Spumoni::JSON *owned)
		{
			Fields::iterator found = fields.find(name);

			if (found != fields.end())
			{
				delete found->second;
				found->second = owned;

				return;
			}

			fields[name] = owned;
		}

		static void WriteFields(const Fields &fields, Spumoni::JSON &object)
		{
			for (Fields::const_iterator i = fields.begin(); i != fields.end(); ++i)
				object.SetOwned(i->first, i->second->Duplicate());
		}

		static bool ReadBounds(const Spumoni::JSON *value, Bounds &out)
		{
			long part[4];

			if (!value || value->GetType() != Spumoni::JSON::Array || value->Size() != 4)
				return false;

			for (size_t i = 0; i < 4; ++i)
			{
				if (!value->At(i)->Integer(part[i]))
					return false;
			}

			out = Bounds(part[0], part[1], part[2], part[3]);

			return true;
		}

		static Spumoni::JSON *MakeInteger(long value)
		{
			char text[32];
			snprintf(text, sizeof text, "%ld", value);

			return Spumoni::JSON::MakeNumber(text);
		}

		static bool NodeList(const Spumoni::JSON *value)
		{
			return value && value->GetType() == Spumoni::JSON::Array;
		}

		Page::Page():
			m_Raw(NULL),
			m_HasChildren(false)
		{
		}

		Page::~Page()
		{
			delete m_Raw;
			Release(m_Fields);

			for (size_t i = 0; i < m_Children.size(); ++i)
				delete m_Children[i];
		}

		Page *Page::FromJSON(const Spumoni::JSON &value)
		{
			Page *page = new Page();

			if (value.GetType() != Spumoni::JSON::Object)
			{
				page->m_Raw = value.Duplicate();

				return page;
			}

			const std::vector<std::string> keys = value.Keys();

			for (size_t i = 0; i < keys.size(); ++i)
			{
				const Spumoni::JSON *field = value.Get(keys[i]);

				if (keys[i] == "children" && NodeList(field))
				{
					page->m_HasChildren = true;

					for (size_t n = 0; n < field->Size(); ++n)
						page->m_Children.push_back(Node::FromJSON(*field->At(n)));

					continue;
				}

				page->m_Fields[keys[i]] = field->Duplicate();
			}

			return page;
		}

		Spumoni::JSON *Page::ToJSON() const
		{
			if (m_Raw)
				return m_Raw->Duplicate();

			Spumoni::JSON *object = Spumoni::JSON::MakeObject();
			WriteFields(m_Fields, *object);

			if (m_HasChildren)
			{
				Spumoni::JSON *children = Spumoni::JSON::MakeArray();

				for (size_t i = 0; i < m_Children.size(); ++i)
					children->AppendOwned(m_Children[i]->ToJSON());

				object->SetOwned("children", children);
			}

			return object;
		}

		Page *Page::Clone() const
		{
			Spumoni::JSONOwner value(ToJSON());

			return FromJSON(*value);
		}

		const Spumoni::JSON *Page::Field(const std::string &name) const
		{
			Fields::const_iterator found = m_Fields.find(name);

			return found == m_Fields.end() ? NULL : found->second;
		}

		bool Page::Text(const std::string &name, Data::ValueRule::Text &out) const
		{
			return Data::ValueRule::Text::Parse(Field(name), out);
		}

		Node::Node():
			m_Raw(NULL),
			m_HasType(false),
			m_HasName(false),
			m_HasBounds(false),
			m_HasChildren(false),
			m_HasPages(false)
		{
		}

		Node::~Node()
		{
			delete m_Raw;
			Release(m_Attributes);
			ClearChildren();

			for (size_t i = 0; i < m_Pages.size(); ++i)
				delete m_Pages[i];
		}

		void Node::ClearChildren()
		{
			for (size_t i = 0; i < m_Children.size(); ++i)
				delete m_Children[i];

			m_Children.clear();
		}

		Node *Node::FromJSON(const Spumoni::JSON &value)
		{
			Node *node = new Node();

			if (value.GetType() != Spumoni::JSON::Object)
			{
				node->m_Raw = value.Duplicate();

				return node;
			}

			const std::vector<std::string> keys = value.Keys();

			for (size_t i = 0; i < keys.size(); ++i)
			{
				const std::string &key = keys[i];
				const Spumoni::JSON *field = value.Get(key);

				if (key == "type" && field->GetType() == Spumoni::JSON::String)
				{
					node->m_HasType = true;
					node->m_Type = field->Text();
				}
				else if (key == "name" && field->GetType() == Spumoni::JSON::String)
				{
					node->m_HasName = true;
					node->m_Name = field->Text();
				}
				else if (key == "bounds" && ReadBounds(field, node->m_Bounds))
					node->m_HasBounds = true;
				else if (key == "children" && NodeList(field))
				{
					node->m_HasChildren = true;

					for (size_t n = 0; n < field->Size(); ++n)
						node->m_Children.push_back(FromJSON(*field->At(n)));
				}
				else if (key == "pages" && NodeList(field))
				{
					node->m_HasPages = true;

					for (size_t n = 0; n < field->Size(); ++n)
						node->m_Pages.push_back(Page::FromJSON(*field->At(n)));
				}
				else
					node->m_Attributes[key] = field->Duplicate();
			}

			return node;
		}

		Spumoni::JSON *Node::ToJSON() const
		{
			if (m_Raw)
				return m_Raw->Duplicate();

			Spumoni::JSON *object = Spumoni::JSON::MakeObject();
			WriteFields(m_Attributes, *object);

			if (m_HasType)
				object->SetOwned("type", Spumoni::JSON::MakeString(m_Type));

			if (m_HasName)
				object->SetOwned("name", Spumoni::JSON::MakeString(m_Name));

			if (m_HasBounds)
			{
				Spumoni::JSON *bounds = Spumoni::JSON::MakeArray();
				bounds->AppendOwned(MakeInteger(m_Bounds.X));
				bounds->AppendOwned(MakeInteger(m_Bounds.Y));
				bounds->AppendOwned(MakeInteger(m_Bounds.W));
				bounds->AppendOwned(MakeInteger(m_Bounds.H));
				object->SetOwned("bounds", bounds);
			}

			if (m_HasChildren)
			{
				Spumoni::JSON *children = Spumoni::JSON::MakeArray();

				for (size_t i = 0; i < m_Children.size(); ++i)
					children->AppendOwned(m_Children[i]->ToJSON());

				object->SetOwned("children", children);
			}

			if (m_HasPages)
			{
				Spumoni::JSON *pages = Spumoni::JSON::MakeArray();

				for (size_t i = 0; i < m_Pages.size(); ++i)
					pages->AppendOwned(m_Pages[i]->ToJSON());

				object->SetOwned("pages", pages);
			}

			return object;
		}

		Node *Node::Clone() const
		{
			Spumoni::JSONOwner value(ToJSON());

			return FromJSON(*value);
		}

		std::string Node::Namespace() const
		{
			const std::string::size_type at = m_Type.find("::");

			return at == std::string::npos ? std::string() : m_Type.substr(0, at);
		}

		std::string Node::ShortType() const
		{
			const std::string::size_type at = m_Type.rfind("::");

			return at == std::string::npos ? m_Type : m_Type.substr(at + 2);
		}

		void Node::SetType(const std::string &type)
		{
			RemoveAttribute("type");
			m_HasType = true;
			m_Type = type;
		}

		void Node::SetName(const std::string &name)
		{
			RemoveAttribute("name");
			m_HasName = true;
			m_Name = name;
		}

		void Node::SetBounds(const Bounds &bounds)
		{
			RemoveAttribute("bounds");
			m_HasBounds = true;
			m_Bounds = bounds;
		}

		bool Node::Has(const std::string &name) const
		{
			if (name == "type")
				return m_HasType || m_Attributes.count(name);

			if (name == "name")
				return m_HasName || m_Attributes.count(name);

			if (name == "bounds")
				return m_HasBounds || m_Attributes.count(name);

			if (name == "children")
				return m_HasChildren || m_Attributes.count(name);

			if (name == "pages")
				return m_HasPages || m_Attributes.count(name);

			return m_Attributes.count(name) != 0;
		}

		const Spumoni::JSON *Node::Attribute(const std::string &name) const
		{
			Fields::const_iterator found = m_Attributes.find(name);

			return found == m_Attributes.end() ? NULL : found->second;
		}

		Spumoni::JSON *Node::Attribute(const std::string &name)
		{
			Fields::iterator found = m_Attributes.find(name);

			return found == m_Attributes.end() ? NULL : found->second;
		}

		std::vector<std::string> Node::AttributeNames() const
		{
			std::vector<std::string> names;

			for (Fields::const_iterator i = m_Attributes.begin(); i != m_Attributes.end(); ++i)
				names.push_back(i->first);

			return names;
		}

		void Node::SetAttribute(const std::string &name, Spumoni::JSON *owned)
		{
			if (name == "type" && owned->GetType() == Spumoni::JSON::String)
			{
				SetType(owned->Text());
				delete owned;

				return;
			}

			if (name == "name" && owned->GetType() == Spumoni::JSON::String)
			{
				SetName(owned->Text());
				delete owned;

				return;
			}

			Bounds bounds;

			if (name == "bounds" && ReadBounds(owned, bounds))
			{
				SetBounds(bounds);
				delete owned;

				return;
			}

			if (name == "children" && NodeList(owned))
			{
				std::vector<Node *> children;

				for (size_t i = 0; i < owned->Size(); ++i)
					children.push_back(FromJSON(*owned->At(i)));

				SetChildren(children);
				delete owned;

				return;
			}

			if (name == "type")
				m_HasType = false;
			else if (name == "name")
				m_HasName = false;
			else if (name == "bounds")
				m_HasBounds = false;
			else if (name == "children")
			{
				ClearChildren();
				m_HasChildren = false;
			}

			Replace(m_Attributes, name, owned);
		}

		Spumoni::JSON *Node::TakeAttribute(const std::string &name)
		{
			Fields::iterator found = m_Attributes.find(name);

			if (found == m_Attributes.end())
				return NULL;

			Spumoni::JSON *value = found->second;
			m_Attributes.erase(found);

			return value;
		}

		void Node::RemoveAttribute(const std::string &name)
		{
			delete TakeAttribute(name);
		}

		void Node::SetChildren(const std::vector<Node *> &owned)
		{
			RemoveAttribute("children");
			ClearChildren();
			m_HasChildren = true;
			m_Children = owned;
		}

		bool Node::String(const std::string &name, std::string &out) const
		{
			if (name == "type" && m_HasType)
			{
				out = m_Type;

				return true;
			}

			if (name == "name" && m_HasName)
			{
				out = m_Name;

				return true;
			}

			const Spumoni::JSON *value = Attribute(name);

			if (!value || value->GetType() != Spumoni::JSON::String)
				return false;

			out = value->Text();

			return true;
		}

		bool Node::Integer(const std::string &name, long &out) const
		{
			const Spumoni::JSON *value = Attribute(name);

			return value && value->Integer(out);
		}

		bool Node::Number(const std::string &name, double &out) const
		{
			const Spumoni::JSON *value = Attribute(name);

			if (!value || value->GetType() != Spumoni::JSON::Number)
				return false;

			out = strtod(value->Text().c_str(), NULL);

			return true;
		}

		bool Node::Boolean(const std::string &name, bool &out) const
		{
			const Spumoni::JSON *value = Attribute(name);

			if (!value || value->GetType() != Spumoni::JSON::Boolean)
				return false;

			out = value->AsBool();

			return true;
		}

		bool Node::Text(const std::string &name, Data::ValueRule::Text &out) const
		{
			return Data::ValueRule::Text::Parse(Attribute(name), out);
		}
	}
}
