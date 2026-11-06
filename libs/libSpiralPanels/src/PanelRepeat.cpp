// SPDX-License-Identifier: GPL-2.0-or-later
#include "PanelRepeat.h"
#include "PanelContracts.h"
#include "PanelDocument.h"
#include "PanelNode.h"
#include "JSON.h"
#include <cstdio>

namespace Spiral
{
	namespace Panels
	{
		typedef Spumoni::JSON JSON;

		static long IntegerOf(const Node &node, const char *name, long fallback)
		{
			long value;
			double number;

			if (node.Integer(name, value))
				return value;

			if (node.Number(name, number))
				return (long)number;

			return fallback;
		}

		static JSON *MakeInteger(long value)
		{
			char text[32];
			snprintf(text, sizeof text, "%ld", value);

			return JSON::MakeNumber(text);
		}

		static bool NameStart(char c)
		{
			return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
		}

		static bool NamePart(char c)
		{
			return NameStart(c) || (c >= '0' && c <= '9');
		}

		std::string Spell(const JSON &value)
		{
			switch (value.GetType())
			{
				case JSON::Boolean:
					return value.AsBool() ? "true" : "false";
				case JSON::Number:
				case JSON::String:
					return value.Text();
				case JSON::Object:
					if (value.Get("default") && value.Get("default")->GetType() == JSON::String)
						return value.Get("default")->Text();

					break;
				default:
					break;
			}

			return value.Stringify(false);
		}

		static std::string SubstituteText(const std::string &text, const Variables &scope)
		{
			std::string out;
			size_t i = 0;

			while (i < text.size())
			{
				if (text[i] == '{' && i + 1 < text.size() && NameStart(text[i + 1]))
				{
					size_t end = i + 2;

					while (end < text.size() && NamePart(text[end]))
						++end;

					if (end < text.size() && text[end] == '}')
					{
						Variables::const_iterator found = scope.find(text.substr(i + 1, end - i - 1));

						if (found != scope.end())
						{
							out += Spell(*found->second);
							i = end + 1;

							continue;
						}
					}
				}

				out += text[i];
				++i;
			}

			return out;
		}

		JSON *Substitute(const JSON &value, const Variables &scope)
		{
			if (value.GetType() == JSON::Object)
			{
				JSON *object = JSON::MakeObject();
				const std::vector<std::string> keys = value.Keys();

				for (size_t i = 0; i < keys.size(); ++i)
					object->SetOwned(keys[i], Substitute(*value.Get(keys[i]), scope));

				return object;
			}

			if (value.GetType() == JSON::Array)
			{
				JSON *array = JSON::MakeArray();

				for (size_t i = 0; i < value.Size(); ++i)
					array->AppendOwned(Substitute(*value.At(i), scope));

				return array;
			}

			if (value.GetType() != JSON::String)
				return value.Duplicate();

			std::string name;

			if (Placeholder(&value, name))
			{
				Variables::const_iterator found = scope.find(name);

				if (found != scope.end())
					return Substitute(*found->second, Variables());
			}

			return JSON::MakeString(SubstituteText(value.Text(), scope));
		}

		Repeat::Repeat(const Node &node):
			m_Node(node),
			m_Items(NULL),
			m_Base(IntegerOf(node, "index_base", 0)),
			m_StepX(IntegerOf(node, "step_x", 0)),
			m_StepY(IntegerOf(node, "step_y", 0)),
			m_X(node.HasBounds() ? node.GetBounds().X : 0),
			m_Y(node.HasBounds() ? node.GetBounds().Y : 0)
		{
			const JSON *items = node.Attribute("items");

			if (items && items->GetType() == JSON::Array)
				m_Items = items;
		}

		bool Repeat::Is(const Node &node)
		{
			return node.HasType() && node.ShortType() == "Repeat";
		}

		size_t Repeat::DefaultCount() const
		{
			if (m_Items)
				return m_Items->Size();

			const long most = IntegerOf(m_Node, "max_count", 1);

			return most > 0 ? (size_t)most : 0;
		}

		std::string Repeat::CountProperty() const
		{
			std::string name;
			m_Node.String("count_property", name);

			return name;
		}

		void Repeat::Origin(size_t copy, long &x, long &y) const
		{
			x = m_X + m_StepX * (long)copy;
			y = m_Y + m_StepY * (long)copy;
		}

		std::vector<Node *> Repeat::Copy(size_t copy) const
		{
			std::vector<Node *> out;
			Spumoni::JSONOwner index(MakeInteger((long)copy + m_Base));
			Spumoni::JSONOwner index0(MakeInteger((long)copy));
			Variables scope;

			if (m_Items && copy < m_Items->Size() && m_Items->At(copy)->GetType() == JSON::Object)
			{
				const JSON *item = m_Items->At(copy);
				const std::vector<std::string> keys = item->Keys();

				for (size_t i = 0; i < keys.size(); ++i)
					scope[keys[i]] = item->Get(keys[i]);
			}

			scope["index"] = index.get();
			scope["index0"] = index0.get();

			for (size_t i = 0; i < m_Node.Children().size(); ++i)
			{
				Spumoni::JSONOwner child(m_Node.Children()[i]->ToJSON());
				Spumoni::JSONOwner copied(Substitute(*child, scope));
				out.push_back(Node::FromJSON(*copied));
			}

			return out;
		}

		static std::vector<Node *> ExpandList(const std::vector<Node *> &nodes, long x, long y, size_t &count);

		static void ExpandInside(Node &node, long x, long y, size_t &count)
		{
			if (node.HasChildren())
			{
				std::vector<Node *> children = node.Children();
				node.Children().clear();
				node.SetChildren(ExpandList(children, x, y, count));
			}

			for (size_t p = 0; p < node.Pages().size(); ++p)
			{
				Page &page = *node.Pages()[p];

				if (!page.HasChildren())
					continue;

				std::vector<Node *> children = page.Children();
				page.Children() = ExpandList(children, x, y, count);
			}
		}

		static std::vector<Node *> ExpandList(const std::vector<Node *> &nodes, long x, long y, size_t &count)
		{
			std::vector<Node *> out;

			for (size_t i = 0; i < nodes.size(); ++i)
			{
				Node *node = nodes[i];

				if (!Repeat::Is(*node))
				{
					ExpandInside(*node, x, y, count);
					out.push_back(node);

					continue;
				}

				++count;
				Repeat repeat(*node);
				const size_t copies = repeat.DefaultCount();

				for (size_t c = 0; c < copies; ++c)
				{
					long cx, cy;
					repeat.Origin(c, cx, cy);
					cx += x;
					cy += y;
					std::vector<Node *> made = repeat.Copy(c);

					for (size_t m = 0; m < made.size(); ++m)
					{
						Node *copy = made[m];

						if (copy->HasBounds())
						{
							Bounds bounds = copy->GetBounds();
							bounds.X += cx;
							bounds.Y += cy;
							copy->SetBounds(bounds);
						}

						std::vector<Node *> one(1, copy);
						std::vector<Node *> expanded = ExpandList(one, cx, cy, count);
						out.insert(out.end(), expanded.begin(), expanded.end());
					}
				}

				delete node;
			}

			return out;
		}

		size_t Expand(Document &document)
		{
			size_t count = 0;
			Node *panel = document.Panel();

			if (!panel || !panel->HasChildren())
				return 0;

			std::vector<Node *> children = panel->Children();
			panel->Children().clear();
			panel->SetChildren(ExpandList(children, 0, 0, count));

			return count;
		}
	}
}
