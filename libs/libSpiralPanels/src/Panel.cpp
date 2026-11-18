// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "Panel.h"
#include "PanelControl.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "PanelSource.h"
#include "JSON.h"
#include "PanelDocument.h"
#include "PanelNode.h"
#include "PanelRepeat.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <algorithm>

namespace Spiral
{
	namespace Panels
	{
		using Spiral::Data::ControlValue;

		static void Offset(ControlView *view, int dx, int dy)
		{
			int x, y, w, h;
			view->GetBounds(x, y, w, h);
			view->SetBounds(x + dx, y + dy, w, h);
		}

		Panel::Panel(PanelHost &host, PanelSource *source):
			m_Sources(NULL),
			m_Host(host),
			m_Source(source),
			m_State(NULL),
			m_Loading(false),
			m_Width(1),
			m_Layout(0),
			m_Mode(0),
			m_AutoWidth(NULL),
			m_Building(false),
			m_PluginId(-1)
		{
			m_Source->Data().Observe(this);
		}

		Panel::~Panel()
		{
			Clear();
			m_Source->Data().Unobserve(this);
			delete m_Source;
		}

		void Panel::Clear()
		{
			m_Layouts.clear();
			m_Layout = 0;
			m_Binder.Clear();
			for (std::map<std::string, Spiral::Data::RowSource *>::iterator i = m_Rows.begin(); i != m_Rows.end(); ++i)
				delete i->second;
			m_Rows.clear();
			m_RowFirst.clear();
			m_Sources = NULL;
			delete m_State;
			m_State = NULL;
			for (size_t i = 0; i < m_Controls.size(); ++i)
				delete m_Controls[i];
			m_Controls.clear();
			for (std::map<std::string, PanelProperty *>::iterator i = m_Properties.begin(); i != m_Properties.end(); ++i)
				delete i->second;
			m_Properties.clear();
			m_Repeats.clear();
			for (size_t i = 0; i < m_Owned.size(); ++i)
				delete m_Owned[i];
			m_Owned.clear();
			m_Unsupported.clear();
			m_Unbound.clear();
			m_Host.ClearViews();
		}

		bool Panel::Build(const Spumoni::JSON &document, std::string *error)
		{
			Spiral::Panels::Document model;
			std::string problem;

			if (!model.Load(document, problem))
			{
				if (error)
					*error = problem;

				return false;
			}

			return Build(model, error);
		}

		bool Panel::Build(const Spiral::Panels::Document &model, std::string *error)
		{
			Clear();

			if (!model.PluginId(m_PluginId))
				m_PluginId = -1;

			Spumoni::JSON *root = model.ToJSON();
			m_Owned.push_back(root);
			const Spumoni::JSON &document = *root;
			const Spumoni::JSON *panel = document.Get("panel");
			if (!panel || panel->GetType() != Spumoni::JSON::Object)
			{
				if (error)
					*error = "no \"panel\" object";
				return false;
			}

			const Spumoni::JSON *properties = document.Get("properties");
			m_Source->Collect();
			m_Source->Describe(m_Properties);
			std::vector<std::string> identities;

			if (properties && properties->GetType() == Spumoni::JSON::Array)
			{
				for (size_t i = 0; i < properties->Size(); ++i)
				{
					PanelProperty *property = PanelProperty::FromJSON(*properties->At(i));
					if (!property)
						continue;

					if (!m_Properties.count(property->Name()))
					{
						m_Properties[property->Name()] = property;
						const bool identity = PanelControl::TextOf(properties->At(i)->Get("omitted")) == "identity";
						if (identity)
							identities.push_back(property->Name());
					}
					else
						delete property;
				}
			}

			const Spumoni::JSON *state = document.Get("state");
			if (state && state->GetType() == Spumoni::JSON::Array)
			{
				m_State = new Spiral::Data::MemorySource();
				std::vector<ControlValue> initial;
				for (size_t i = 0; i < state->Size(); ++i)
				{
					PanelProperty *property = PanelProperty::FromJSON(*state->At(i));
					if (!property)
						continue;
					if (m_Properties.count(property->Name()))
						delete m_Properties[property->Name()];
					m_Properties[property->Name()] = property;
					m_State->AddField(property->Name(), property->Type, false, property->Label);
					ControlValue value(property->Default());
					if (property->Type == PanelProperty::String)
						value = ControlValue(std::string());
					else if (property->Type == PanelProperty::Boolean)
						value = ControlValue(property->Default() != 0);
					else
						value.type = property->Type;
					initial.push_back(value);
				}
				m_State->AddRow(initial);
				m_State->Seek(0);
				m_State->Observe(this);
			}

			for (std::map<std::string, PanelProperty *>::iterator i = m_Properties.begin(); i != m_Properties.end(); ++i)
			{
				if (std::find(identities.begin(), identities.end(), i->first) != identities.end() || IsState(*i->second))
					continue;

				if (!i->second->IsCommand() && m_Source->Data().FindField(i->first) < 0 && !m_Source->AddElement(*i->second))
					m_Unbound.push_back(i->first + ": no property named like it");
			}

			const int width = (int)PanelControl::NumberOf(panel->Get("width"), 100);
			const int height = (int)PanelControl::NumberOf(panel->Get("height"), 100);
			m_Host.Resize(width, height);
			m_Width = width;
			m_Help = PanelControl::TextOf(panel->Get("help"));
			m_AutoWidth = panel->Get("auto_width");

			BuildSources(*panel);

			if (model.Panel() && model.Panel()->HasChildren())
			{
				int originX, originY;
				m_Host.Origin(originX, originY);
				m_Host.BeginBuild();
				m_Building = true;
				BuildNodes(model.Panel()->Children(), originX, originY);
				m_Building = false;
				for (size_t i = 0; i < m_Controls.size(); ++i)
					m_Controls[i]->VerifyBindings();
				m_Host.EndBuild();
			}
			ReadLayouts(*panel);
			ReadModes(*panel);
			Bind();
			return true;
		}

		namespace
		{
			// A table a device keeps in one channel.
			class ChannelTable: public Spiral::Data::TableFeed
			{
			public:
				ChannelTable(const PanelSource &source, const std::string &channel): m_Source(source), m_Channel(channel), m_Revision(0) {}

				bool Cells(std::vector<double> &out) const
				{
					return m_Source.ReadTableSince(m_Channel, m_Revision, out);
				}

			private:
				const PanelSource &m_Source;
				std::string m_Channel;
				mutable unsigned long m_Revision;
			};
		}

		// A panel's row sources, before the controls that name them: the
		// fields their rows share or the table they are read from, how many
		// rows, and the property a selector moves the cursor by.
		void Panel::BuildSources(const Spumoni::JSON &panel)
		{
			const Spumoni::JSON *sources = panel.Get("sources");

			m_Sources = sources;

			if (!sources || sources->GetType() != Spumoni::JSON::Object)
				return;

			const std::vector<std::string> names = sources->Keys();

			for (size_t i = 0; i < names.size(); ++i)
			{
				const Spumoni::JSON &written = *sources->Get(names[i]);
				const Spumoni::JSON *fields = written.Get("fields"), *count = written.Get("count");
				const long first = (long)PanelControl::NumberOf(written.Get("first"), 0);
				Spiral::Data::RowSource *rows = new Spiral::Data::RowSource(&m_Source->Data(), first);

				m_Rows[names[i]] = rows;
				m_RowFirst[names[i]] = first;

				const std::string table = PanelControl::TextOf(written.Get("table"));
				const bool named = fields && fields->GetType() == Spumoni::JSON::Array;

				// Rows over a table: its fields name the columns, or there
				// are so many of them, numbered.
				if (!table.empty() && named)
				{
					std::vector<std::string> columns;

					for (size_t f = 0; f < fields->Size(); ++f)
						columns.push_back(PanelControl::TextOf(fields->At(f)));

					rows->TableFrom(new ChannelTable(*m_Source, table), columns);
				}
				else if (!table.empty())
					rows->TableFrom(new ChannelTable(*m_Source, table), (int)PanelControl::NumberOf(written.Get("columns"), 1));

				for (size_t f = 0; table.empty() && named && f < fields->Size(); ++f)
					if (rows->AddField(PanelControl::TextOf(fields->At(f))) < 0)
						m_Unbound.push_back(names[i] + "." + PanelControl::TextOf(fields->At(f)) + ": no row has such a property");

				// A number of rows, or the property that holds it.
				if (count && count->GetType() == Spumoni::JSON::String)
				{
					if (!rows->CountFrom(count->Text()))
						m_Unbound.push_back(names[i] + ": no count property " + count->Text());
				}
				else if (count)
					rows->SetCount((long)PanelControl::NumberOf(count, 0));

				const std::string position = PanelControl::TextOf(written.Get("position"));

				if (!position.empty() && !rows->PositionFrom(position))
					m_Unbound.push_back(names[i] + ": no position property " + position);

				rows->Observe(this);
			}
		}

		Spiral::Data::RowSource *Panel::Rows(const std::string &source) const
		{
			const std::map<std::string, Spiral::Data::RowSource *>::const_iterator found = m_Rows.find(source);

			return found == m_Rows.end() ? NULL : found->second;
		}

		long Panel::FirstRow(const std::string &source) const
		{
			const std::map<std::string, long>::const_iterator found = m_RowFirst.find(source);

			return found == m_RowFirst.end() ? 0 : found->second;
		}

		long Panel::RowNumber(const std::string &source) const
		{
			const Spiral::Data::RowSource *rows = Rows(source);

			return FirstRow(source) + (rows ? rows->Position() : 0);
		}

		const PanelProperty *Panel::FindOnRow(const std::string &source, const std::string &name) const
		{
			const Spiral::Data::RowSource *rows = source.empty() ? NULL : Rows(source);

			if (rows && rows->FindField(name) >= 0)
			{
				char number[32];

				snprintf(number, sizeof(number), "%ld", RowNumber(source));

				if (const PanelProperty *found = FindProperty(name + number))
					return found;
			}

			return FindProperty(name);
		}

		// A source's cursor moved. A control bound to the rows has already
		// been told by its binding; the property it reads its range and its
		// unbound value from moves here, to the new row's.
		void Panel::FollowRows(Spiral::Data::DataSource &moved)
		{
			for (std::map<std::string, Spiral::Data::RowSource *>::iterator rows = m_Rows.begin(); rows != m_Rows.end(); ++rows)
			{
				if (rows->second != &moved)
					continue;

				char number[32];

				snprintf(number, sizeof(number), "%ld", RowNumber(rows->first));

				const bool loading = m_Loading;

				m_Loading = true;

				for (size_t i = 0; i < m_Controls.size(); ++i)
				{
					PanelControl &control = *m_Controls[i];

					if (control.RowSourceName() != rows->first)
						continue;

					const PanelProperty *to = control.m_RowField.empty() ? NULL : FindProperty(control.m_RowField + number);

					if (to)
						control.m_Property = to;

					// A limit or a scale it takes from one of the rows'
					// fields is the new row's too.
					if (!control.m_MinName.empty())
						control.m_MinProperty = FindOnRow(rows->first, control.m_MinName);

					if (!control.m_MaxName.empty())
						control.m_MaxProperty = FindOnRow(rows->first, control.m_MaxName);

					if (!control.m_PowerName.empty())
						control.m_ScalePower = FindOnRow(rows->first, control.m_PowerName);

					control.RowMoved();

					if (!control.Bound())
						control.Load();
				}

				m_Loading = loading;
			}
		}

		void Panel::Bind()
		{
			for (size_t i = 0; i < m_Controls.size(); ++i)
			{
				const PanelProperty *property = m_Controls[i]->Property();
				Spiral::Data::RowSource *rows = Rows(m_Controls[i]->RowSourceName());

				// A control on a row source binds to the rows' field, so its
				// binding follows the cursor.
				if (rows && property && rows->FindField(m_Controls[i]->m_RowField) >= 0)
					m_Controls[i]->BindTo(m_Binder, *rows);
				else
					m_Controls[i]->BindTo(m_Binder, property ? SourceOf(*property) : m_Source->Data());
			}
		}

		bool Panel::IsState(const PanelProperty &property) const
		{
			return m_State && m_State->FindField(property.Name()) >= 0;
		}

		Spiral::Data::DataSource &Panel::SourceOf(const PanelProperty &property) const
		{
			if (IsState(property))
				return *m_State;
			return m_Source->Data();
		}

		void Panel::SaveState(std::map<std::string, ControlValue> &state) const
		{
			for (int i = 0; m_State && i < m_State->FieldCount(); ++i)
			{
				ControlValue value;

				if (m_State->Get(i, value))
					state[m_State->Field(i).Key] = value;
			}
		}

		void Panel::LoadState(const std::map<std::string, ControlValue> &state)
		{
			for (std::map<std::string, ControlValue>::const_iterator i = state.begin(); i != state.end(); ++i)
			{
				const int field = m_State ? m_State->FindField(i->first) : -1;

				if (field < 0)
					continue;

				ControlValue value = i->second;

				if (value.type == ControlValue::Number && m_State->Field(field).Type == ControlValue::Integer)
					value.type = ControlValue::Integer;

				m_State->Set(field, value);
			}
		}

		void Panel::BuildChildren(const Spumoni::JSON &children, int originX, int originY)
		{
			if (children.GetType() != Spumoni::JSON::Array)
				return;

			std::vector<Spiral::Panels::Node *> nodes;

			for (size_t i = 0; i < children.Size(); ++i)
				nodes.push_back(Spiral::Panels::Node::FromJSON(*children.At(i)));

			BuildNodes(nodes, originX, originY);

			for (size_t i = 0; i < nodes.size(); ++i)
				delete nodes[i];
		}

		void Panel::BuildNodes(const std::vector<Spiral::Panels::Node *> &nodes, int originX, int originY)
		{
			for (size_t i = 0; i < nodes.size(); ++i)
			{
				const Spiral::Panels::Node &node = *nodes[i];
				std::string needed;

				if (node.String("if_property", needed) && !needed.empty() && !FindProperty(needed))
					continue;

				double expand = 0;

				if (node.Number("expand_height", expand) && (int)expand > m_Host.Height())
					m_Host.Resize(m_Host.Width(), (int)expand);

				if (node.Number("expand_width", expand) && (int)expand > m_Host.Width())
					m_Host.Resize((int)expand, m_Host.Height());

				if (Spiral::Panels::Repeat::Is(node))
				{
					BuildRepeat(node, originX, originY);

					continue;
				}

				Spumoni::JSON *json = node.ToJSON();
				m_Owned.push_back(json);
				PanelControl *control = PanelControlRegistry::Get().Create(node.ShortType(), *this, *json);

				if (!control)
				{
					m_Unsupported.push_back(node.Type());

					continue;
				}

				if (control->View())
					Offset(control->View(), originX, originY);

				control->ReadHandlers();
				m_Controls.push_back(control);
			}
		}

		void Panel::BuildRepeat(const Spiral::Panels::Node &node, int originX, int originY)
		{
			const Spiral::Panels::Repeat model(node, m_Sources);
			Repeat repeat;
			repeat.Count = FindProperty(model.CountProperty());
			repeat.Shown = -1;
			repeat.StepX = (int)model.StepX();
			repeat.StepY = (int)model.StepY();
			repeat.AutoWidth = m_AutoWidth ? m_AutoWidth->AsBool() : repeat.StepX > 0;
			double most = repeat.Count && repeat.Count->HasRange() ? repeat.Count->Max() : 1;

			// By its items, by the count its panel states, or with no count
			// property the fixed number its row source has.
			if (model.ByItems() || (!repeat.Count && !node.Has("max_count")))
				most = (double)model.DefaultCount();
			else
				node.Number("max_count", most);

			if (!node.HasChildren())
				return;

			for (int i = 0; i < (int)most; ++i)
			{
				std::vector<Spiral::Panels::Node *> copy = model.Copy(i);
				long copyX, copyY;
				model.Origin(i, copyX, copyY);
				const size_t before = m_Controls.size();
				BuildNodes(copy, originX + (int)copyX, originY + (int)copyY);

				for (size_t c = 0; c < copy.size(); ++c)
					delete copy[c];

				std::vector<ControlView *> views;

				for (size_t c = before; c < m_Controls.size(); ++c)
				{
					if (m_Controls[c]->View())
						views.push_back(m_Controls[c]->View());
				}

				repeat.Copies.push_back(views);
			}

			m_Repeats.push_back(repeat);
		}

		void Panel::ReadModes(const Spumoni::JSON &panel)
		{
			const Spumoni::JSON *modes = panel.Get("modes");
			if (!modes || modes->GetType() != Spumoni::JSON::Array)
				return;
			for (size_t i = 0; i < modes->Size(); ++i)
			{
				const Spumoni::JSON *node = modes->At(i);
				if (!node || node->GetType() != Spumoni::JSON::Object)
					continue;
				PanelMode mode;
				mode.Name = PanelControl::TextOf(node->Get("name"));
				mode.Property = PanelControl::TextOf(node->Get("property"));
				mode.Value = PanelControl::NumberOf(node->Get("value"), 0);
				m_Modes.push_back(mode);
				const PanelProperty *property = FindProperty(mode.Property);
				if (property && Read(*property) == mode.Value)
					m_Mode = m_Modes.size() - 1;
			}
		}

		void Panel::SelectMode(size_t index)
		{
			if (index >= m_Modes.size())
				return;
			const PanelProperty *property = FindProperty(m_Modes[index].Property);
			if (!property)
				return;
			m_Mode = index;
			Write(*property, m_Modes[index].Value);
		}

		void Panel::ReadLayouts(const Spumoni::JSON &panel)
		{
			const Spumoni::JSON *layouts = panel.Get("layouts");
			if (!layouts || layouts->GetType() != Spumoni::JSON::Array)
				return;
			for (size_t i = 0; i < layouts->Size(); ++i)
			{
				const Spumoni::JSON *node = layouts->At(i);
				if (!node || node->GetType() != Spumoni::JSON::Object)
					continue;
				PanelLayout layout;
				layout.Name = PanelControl::TextOf(node->Get("name"));
				layout.Width = (int)PanelControl::NumberOf(node->Get("width"), m_Host.Width());
				layout.Height = (int)PanelControl::NumberOf(node->Get("height"), m_Host.Height());
				const Spumoni::JSON *controls = node->Get("controls");
				if (controls && controls->GetType() == Spumoni::JSON::Object)
				{
					const std::vector<std::string> names = controls->Keys();
					for (size_t n = 0; n < names.size(); ++n)
					{
						const Spumoni::JSON *bounds = controls->Get(names[n]);
						if (!bounds || bounds->GetType() != Spumoni::JSON::Array || bounds->Size() != 4)
							continue;
						Bounds b = { (int)PanelControl::NumberOf(bounds->At(0), 0), (int)PanelControl::NumberOf(bounds->At(1), 0),
						             (int)PanelControl::NumberOf(bounds->At(2), 0), (int)PanelControl::NumberOf(bounds->At(3), 0) };
						layout.Controls[names[n]] = b;
					}
				}
				m_Layouts.push_back(layout);
			}
			if (!m_Layouts.empty())
				for (size_t i = 0; i < m_Controls.size(); ++i)
					if (m_Controls[i]->View() && !m_Layouts[0].Controls.count(m_Controls[i]->Name()))
					{
						int originX, originY, x, y, w, h;
						m_Host.Origin(originX, originY);
						m_Controls[i]->View()->GetBounds(x, y, w, h);
						Bounds b = { x - originX, y - originY, w, h };
						m_Layouts[0].Controls[m_Controls[i]->Name()] = b;
					}

			// The panel opens in the first layout, which may place some
			// controls with no size: those are another layout's.
			if (!m_Layouts.empty())
				for (size_t i = 0; i < m_Controls.size(); ++i)
				{
					std::map<std::string, Bounds>::const_iterator b = m_Layouts[0].Controls.find(m_Controls[i]->Name());

					if (m_Controls[i]->View() && b != m_Layouts[0].Controls.end() && (b->second.W <= 0 || b->second.H <= 0))
						m_Controls[i]->View()->Show(false);
				}
		}

		void Panel::ApplyLayout(size_t index)
		{
			if (index >= m_Layouts.size())
				return;
			const PanelLayout &layout = m_Layouts[index];
			m_Host.Resize(layout.Width, layout.Height);
			m_Width = layout.Width;
			for (size_t r = 0; r < m_Repeats.size(); ++r)
				m_Repeats[r].Shown = -1;
			int originX, originY;
			m_Host.Origin(originX, originY);

			for (size_t i = 0; i < m_Controls.size(); ++i)
			{
				ControlView *view = m_Controls[i]->View();
				if (!view)
					continue;
				std::map<std::string, Bounds>::const_iterator b = layout.Controls.find(m_Controls[i]->Name());
				if (b == layout.Controls.end())
					b = m_Layouts[0].Controls.find(m_Controls[i]->Name());
				if (b == m_Layouts[0].Controls.end())
					continue;
				view->SetBounds(originX + b->second.X, originY + b->second.Y, b->second.W, b->second.H);
				// A control a layout gives no size is not shown in it.
				view->Show(b->second.W > 0 && b->second.H > 0);
			}
			m_Layout = index;
			m_Host.Redraw();
		}

		int Panel::RepeatCount(const Repeat &repeat) const
		{
			return repeat.Count ? (int)Read(*repeat.Count) : (int)repeat.Copies.size();
		}

		void Panel::ShowRepeats()
		{
			int width = m_Width;
			int originX, originY;
			m_Host.Origin(originX, originY);

			for (size_t r = 0; r < m_Repeats.size(); ++r)
			{
				Repeat &repeat = m_Repeats[r];
				const int count = RepeatCount(repeat);
				repeat.Shown = count;
				for (size_t i = 0; i < repeat.Copies.size(); ++i)
					for (size_t w = 0; w < repeat.Copies[i].size(); ++w)
					{
						ControlView *view = repeat.Copies[i][w];
						view->Show((int)i < count);

						if ((int)i < count && repeat.AutoWidth)
						{
							int x, y, wide, high;
							view->GetBounds(x, y, wide, high);

							if (x + wide - originX > width)
								width = x + wide - originX;
						}
					}
			}
			if (width != m_Host.Width())
				m_Host.Resize(width, m_Host.Height());
		}

		void Panel::Load()
		{
			m_Loading = true;
			m_Source->Refresh();
			for (std::map<std::string, Spiral::Data::RowSource *>::iterator rows = m_Rows.begin(); rows != m_Rows.end(); ++rows)
				rows->second->Poll();
			ShowRepeats();
			m_Binder.Refresh();
			for (size_t i = 0; i < m_Controls.size(); ++i)
			{
				if (!m_Controls[i]->Bound())
					m_Controls[i]->Load();
				m_Controls[i]->Tick();
			}
			m_Loading = false;
		}

		void Panel::Refresh()
		{
			m_Loading = true;
			m_Source->Refresh();
			// A device source that tells no one of a moved selector is asked.
			for (std::map<std::string, Spiral::Data::RowSource *>::iterator rows = m_Rows.begin(); rows != m_Rows.end(); ++rows)
				rows->second->Poll();
			for (size_t r = 0; r < m_Repeats.size(); ++r)
				if (RepeatCount(m_Repeats[r]) != m_Repeats[r].Shown)
				{
					ShowRepeats();
					break;
				}
			for (size_t i = 0; i < m_Controls.size(); ++i)
			{
				m_Controls[i]->Refresh();
				m_Controls[i]->Tick();
			}
			m_Loading = false;
		}

		const PanelProperty *Panel::FindProperty(const std::string &name) const
		{
			std::map<std::string, PanelProperty *>::const_iterator i = m_Properties.find(name);
			return i == m_Properties.end() ? NULL : i->second;
		}

		PanelControl *Panel::FindControl(const std::string &name) const
		{
			for (size_t i = 0; i < m_Controls.size(); ++i)
			{
				if (m_Controls[i]->Name() == name)
					return m_Controls[i];
			}
			return NULL;
		}

		bool Panel::Write(const PanelProperty &property, double value)
		{
			if (m_Loading || property.IsReadOnly())
				return false;

			Spiral::Data::DataSource &source = SourceOf(property);
			return source.Set(source.FindField(property.Name()), ControlValue(value));
		}

		void Panel::DataChanged(Spiral::Data::DataSource &source, int field)
		{
			if (field < 0)
			{
				FollowRows(source);
				return;
			}
			if (source.FieldCount() <= field)
				return;
			const PanelProperty *property = FindProperty(source.Field(field).Key);
			if (!property)
				return;
			const bool loading = m_Loading;
			m_Loading = true;
			for (size_t i = 0; i < m_Controls.size(); ++i)
				if (m_Controls[i]->DependsOn(*property))
					m_Controls[i]->Reload();
			m_Loading = loading;
		}

		void Panel::DataSourceDestroyed(Spiral::Data::DataSource &)
		{
		}

		bool Panel::Command(const std::string &name, const std::vector<std::string> &args)
		{
			return !m_Loading && m_Source->Command(name, args);
		}

		void Panel::WriteText(const PanelProperty &property, const std::string &text)
		{
			if (m_Loading || property.IsReadOnly())
				return;
			Spiral::Data::DataSource &source = SourceOf(property);
			source.Set(source.FindField(property.Name()), ControlValue(text));
		}

		double Panel::Read(const PanelProperty &property) const
		{
			ControlValue value;
			Spiral::Data::DataSource &source = SourceOf(property);
			if (!source.Get(source.FindField(property.Name()), value))
				return property.Default();
			return value.type == ControlValue::Boolean ? (value.boolean ? 1 : 0) : value.number;
		}

		std::string Panel::ReadText(const PanelProperty &property) const
		{
			ControlValue value;
			Spiral::Data::DataSource &source = SourceOf(property);
			return source.Get(source.FindField(property.Name()), value) ? value.text : std::string();
		}

		bool Panel::Scope::HasField(const std::string &name) const
		{
			return m_Panel.FindOnRow(m_Rows, name) != NULL;
		}

		double Panel::Scope::Field(const std::string &name) const
		{
			const PanelProperty *property = m_Panel.FindOnRow(m_Rows, name);
			return property ? m_Panel.Read(*property) : 0;
		}

		bool Panel::Scope::HasAttribute(const std::string &control, const std::string &attribute) const
		{
			if (m_Panel.Building())
				return true;
			const PanelControl *source = m_Panel.FindControl(control);
			double value;
			return source && source->GetAttribute(attribute, value);
		}

		double Panel::Scope::Attribute(const std::string &control, const std::string &attribute) const
		{
			const PanelControl *source = m_Panel.FindControl(control);
			double value = 0;
			if (source)
				source->GetAttribute(attribute, value);
			return value;
		}

		bool Panel::HandlerScope::HasField(const std::string &name) const
		{
			return m_Panel.FindOnRow(m_Control.RowSourceName(), name) != NULL;
		}

		bool Panel::HandlerScope::HasControl(const std::string &name) const
		{
			return m_Panel.Building() || m_Panel.FindControl(name) != NULL;
		}

		bool Panel::HandlerScope::HasAttribute(const std::string &control, const std::string &attribute) const
		{
			if (m_Panel.Building())
				return true;
			const PanelControl *source = m_Panel.FindControl(control);
			double value;
			return source && source->GetAttribute(attribute, value);
		}

		bool Panel::HandlerScope::CanSetAttribute(const std::string &control, const std::string &) const
		{
			return m_Panel.Building() || m_Panel.FindControl(control) != NULL;
		}

		bool Panel::HandlerScope::Event(const std::string &name, std::vector<std::string> &arguments) const
		{
			std::vector<PanelControl::Event> events;
			m_Control.DeclareEvents(events);
			for (size_t i = 0; i < events.size(); ++i)
				if (name == events[i].name)
				{
					arguments.clear();
					for (size_t a = 0; events[i].arguments[a]; ++a)
						arguments.push_back(events[i].arguments[a]);
					return true;
				}
			return false;
		}
	}
}
