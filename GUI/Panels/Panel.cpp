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
#include "DeviceSource.h"
#include "PatchDescription.h"
#include "JSON.h"
#include <FL/Fl_Group.H>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <algorithm>

namespace SpiralPanels
{
	using Spiral::Data::ControlValue;

	static void Offset(Fl_Widget *widget, int dx, int dy)
	{
		widget->position(widget->x() + dx, widget->y() + dy);
	}

	Panel::Panel(int x, int y, SpiralPlugin *plugin):
		Fl_Group(x, y, 1, 1),
		m_Source(new DeviceSource(plugin)),
		m_State(NULL),
		m_Loading(false),
		m_Width(1),
		m_Layout(0),
		m_Mode(0),
		m_AutoWidth(NULL),
		m_Building(false)
	{
		end();
		m_Source->Observe(this);
	}

	Panel::~Panel()
	{
		Clear();
		m_Source->Unobserve(this);
		delete m_Source;
	}

	void Panel::Clear()
	{
		m_Layouts.clear();
		m_Layout = 0;
		m_Binder.Clear();
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
		clear();
	}

	bool Panel::Build(const Spumoni::JSON &document, std::string *error)
	{
		Clear();

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

			if (!i->second->IsCommand() && m_Source->FindField(i->first) < 0 && !m_Source->AddElement(*i->second))
				m_Unbound.push_back(i->first + ": no property named like it");
		}

		const int width = (int)PanelControl::NumberOf(panel->Get("width"), 100);
		const int height = (int)PanelControl::NumberOf(panel->Get("height"), 100);
		size(width, height);
		m_Width = width;
		m_Help = PanelControl::TextOf(panel->Get("help"));
		m_AutoWidth = panel->Get("auto_width");

		const Spumoni::JSON *children = panel->Get("children");
		if (children)
		{
			begin();
			m_Building = true;
			BuildChildren(*children, x(), y());
			m_Building = false;
			for (size_t i = 0; i < m_Controls.size(); ++i)
				m_Controls[i]->VerifyBindings();
			end();
		}
		ReadLayouts(*panel);
		ReadModes(*panel);
		Bind();
		return true;
	}

	void Panel::Bind()
	{
		for (size_t i = 0; i < m_Controls.size(); ++i)
		{
			const PanelProperty *property = m_Controls[i]->Property();
			m_Controls[i]->BindTo(m_Binder, property ? SourceOf(*property) : *m_Source);
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
		return *m_Source;
	}

	void Panel::SaveState(std::map<std::string, Spiral::Core::StateValue> &state) const
	{
		using Spiral::Core::StateValue;
		for (int i = 0; m_State && i < m_State->FieldCount(); ++i)
		{
			ControlValue value;
			if (!m_State->Get(i, value))
				continue;
			const std::string &key = m_State->Field(i).Key;
			if (value.type == ControlValue::String)
				state[key] = StateValue(StateValue::String, value.text);
			else if (value.type == ControlValue::Boolean)
				state[key] = StateValue(StateValue::Boolean, value.boolean ? "true" : "false");
			else
			{
				std::ostringstream text;
				text << value.number;
				state[key] = StateValue(StateValue::Number, text.str());
			}
		}
	}

	void Panel::LoadState(const std::map<std::string, Spiral::Core::StateValue> &state)
	{
		using Spiral::Core::StateValue;
		for (std::map<std::string, StateValue>::const_iterator i = state.begin(); i != state.end(); ++i)
		{
			const int field = m_State ? m_State->FindField(i->first) : -1;
			if (field < 0)
				continue;
			ControlValue value;
			if (i->second.Type == StateValue::String)
				value = ControlValue(i->second.Text);
			else if (i->second.Type == StateValue::Boolean)
				value = ControlValue(i->second.Text == "true");
			else
			{
				value = ControlValue(std::strtod(i->second.Text.c_str(), NULL));
				if (m_State->Field(field).Type == ControlValue::Integer)
					value.type = ControlValue::Integer;
			}
			m_State->Set(field, value);
		}
	}

	void Panel::BuildChildren(const Spumoni::JSON &children, int originX, int originY)
	{
		if (children.GetType() != Spumoni::JSON::Array)
			return;

		for (size_t i = 0; i < children.Size(); ++i)
		{
			const Spumoni::JSON &node = *children.At(i);
			const std::string needed = PanelControl::TextOf(node.Get("if_property"));
			if (!needed.empty() && !FindProperty(needed))
				continue;
			const int expand = (int)PanelControl::NumberOf(node.Get("expand_height"), 0);
			if (expand > h())
				Fl_Widget::resize(x(), y(), w(), expand);
			const std::string type = PanelControl::TextOf(node.Get("type"));
			if (type == "Repeat")
			{
				BuildRepeat(node, originX, originY);
				continue;
			}
			PanelControl *control = PanelControlRegistry::Get().Create(type, *this, node);
			if (!control)
			{
				m_Unsupported.push_back(type);
				continue;
			}
			if (control->Widget())
			{
				Offset(control->Widget(), originX, originY);
			}
			control->ReadHandlers();
			m_Controls.push_back(control);
		}
	}

	void Panel::BuildRepeat(const Spumoni::JSON &node, int originX, int originY)
	{
		Repeat repeat;
		repeat.Count = FindProperty(PanelControl::TextOf(node.Get("count_property")));
		repeat.Shown = -1;
		repeat.StepX = (int)PanelControl::NumberOf(node.Get("step_x"), 0);
		repeat.StepY = (int)PanelControl::NumberOf(node.Get("step_y"), 0);
		repeat.AutoWidth = m_AutoWidth ? m_AutoWidth->AsBool() : repeat.StepX > 0;
		const int base = (int)PanelControl::NumberOf(node.Get("index_base"), 0);
		const int most = (int)PanelControl::NumberOf(node.Get("max_count"),
			repeat.Count && repeat.Count->HasRange() ? repeat.Count->Max() : 1);
		const Spumoni::JSON *children = node.Get("children");
		const Spumoni::JSON *bounds = node.Get("bounds");
		const int bx = bounds && bounds->Size() > 0 ? (int)PanelControl::NumberOf(bounds->At(0), 0) : 0;
		const int by = bounds && bounds->Size() > 1 ? (int)PanelControl::NumberOf(bounds->At(1), 0) : 0;
		if (!children)
			return;
		const std::string text = children->Stringify(false);
		for (int i = 0; i < most; ++i)
		{
			char index[16], index0[16];
			snprintf(index, sizeof(index), "%d", i + base);
			snprintf(index0, sizeof(index0), "%d", i);
			std::string copy = text;
			for (size_t at = copy.find("\"{index}\""); at != std::string::npos; at = copy.find("\"{index}\"", at))
				copy.replace(at, 9, index);
			for (size_t at = copy.find("\"{index0}\""); at != std::string::npos; at = copy.find("\"{index0}\"", at))
				copy.replace(at, 10, index0);
			for (size_t at = copy.find("{index}"); at != std::string::npos; at = copy.find("{index}", at))
				copy.replace(at, 7, index);
			for (size_t at = copy.find("{index0}"); at != std::string::npos; at = copy.find("{index0}", at))
				copy.replace(at, 8, index0);
			Spumoni::JSON *parsed = Spumoni::ParseJSONText(copy);
			if (!parsed)
				continue;
			m_Owned.push_back(parsed);
			const size_t before = m_Controls.size();
			BuildChildren(*parsed, originX + bx + repeat.StepX * i, originY + by + repeat.StepY * i);
			std::vector<Fl_Widget *> widgets;
			for (size_t c = before; c < m_Controls.size(); ++c)
				if (m_Controls[c]->Widget())
					widgets.push_back(m_Controls[c]->Widget());
			repeat.Copies.push_back(widgets);
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
			layout.Width = (int)PanelControl::NumberOf(node->Get("width"), w());
			layout.Height = (int)PanelControl::NumberOf(node->Get("height"), h());
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
				if (m_Controls[i]->Widget() && !m_Layouts[0].Controls.count(m_Controls[i]->Name()))
				{
					Fl_Widget *widget = m_Controls[i]->Widget();
					Bounds b = { widget->x() - x(), widget->y() - y(), widget->w(), widget->h() };
					m_Layouts[0].Controls[m_Controls[i]->Name()] = b;
				}
	}

	void Panel::ApplyLayout(size_t index)
	{
		if (index >= m_Layouts.size())
			return;
		const PanelLayout &layout = m_Layouts[index];
		size(layout.Width, layout.Height);
		m_Width = layout.Width;
		for (size_t r = 0; r < m_Repeats.size(); ++r)
			m_Repeats[r].Shown = -1;
		for (size_t i = 0; i < m_Controls.size(); ++i)
		{
			Fl_Widget *widget = m_Controls[i]->Widget();
			if (!widget)
				continue;
			std::map<std::string, Bounds>::const_iterator b = layout.Controls.find(m_Controls[i]->Name());
			if (b == layout.Controls.end())
				b = m_Layouts[0].Controls.find(m_Controls[i]->Name());
			if (b == m_Layouts[0].Controls.end())
				continue;
			widget->resize(x() + b->second.X, y() + b->second.Y, b->second.W, b->second.H);
		}
		m_Layout = index;
		redraw();
	}

	int Panel::RepeatCount(const Repeat &repeat) const
	{
		return repeat.Count ? (int)Read(*repeat.Count) : (int)repeat.Copies.size();
	}

	void Panel::ShowRepeats()
	{
		int width = m_Width;
		for (size_t r = 0; r < m_Repeats.size(); ++r)
		{
			Repeat &repeat = m_Repeats[r];
			const int count = RepeatCount(repeat);
			repeat.Shown = count;
			for (size_t i = 0; i < repeat.Copies.size(); ++i)
				for (size_t w = 0; w < repeat.Copies[i].size(); ++w)
				{
					Fl_Widget *widget = repeat.Copies[i][w];
					if ((int)i < count)
					{
						widget->show();
						if (repeat.AutoWidth && widget->x() + widget->w() - x() > width)
							width = widget->x() + widget->w() - x();
					}
					else
						widget->hide();
				}
		}
		if (width != w())
		{
			Fl_Widget::resize(x(), y(), width, h());
			if (parent())
				parent()->redraw();
		}
	}

	void Panel::Load()
	{
		m_Loading = true;
		m_Source->Refresh();
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

	SpiralPlugin *Panel::Plugin() const
	{
		return m_Source ? m_Source->Plugin() : NULL;
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
		return m_Panel.FindProperty(name) != NULL;
	}

	double Panel::Scope::Field(const std::string &name) const
	{
		const PanelProperty *property = m_Panel.FindProperty(name);
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
		return m_Panel.FindProperty(name) != NULL;
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
