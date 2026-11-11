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
#include "PanelControl.h"
#include <cstdio>
#include "Panel.h"
#include "PanelProperty.h"
#include "JSON.h"
#include "PanelSource.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Spiral
{
	namespace Panels
	{
		using Spiral::Data::ControlValue;

		static const Spiral::Data::Attribute NumberValue = { "value", ControlValue::Number, true };
		static const Spiral::Data::Attribute TextValue = { "value", ControlValue::String, true };

		class PanelConverter: public Spiral::Data::ValueConverter
		{
		public:
			explicit PanelConverter(const PanelControl &control): m_Control(control) {}

			virtual bool ToControl(const ControlValue &field, ControlValue::Type to, ControlValue &out) const
			{
				if (to == ControlValue::String || field.type == ControlValue::String)
					return Spiral::Data::Binding::Convert(field, to, out);
				out = ControlValue(m_Control.Mapped(field.type == ControlValue::Boolean ? (field.boolean ? 1 : 0) : field.number));
				return true;
			}

			virtual bool ToField(const ControlValue &control, ControlValue::Type to, ControlValue &out) const
			{
				if (to == ControlValue::String || control.type == ControlValue::String)
					return Spiral::Data::Binding::Convert(control, to, out);
				return Spiral::Data::Binding::Convert(ControlValue(m_Control.Unmapped(control.number)), to, out);
			}

		private:
			const PanelControl &m_Control;
		};

		std::string PanelControl::TextOf(const Spumoni::JSON *node)
		{
			if (!node)
				return std::string();
			if (node->GetType() == Spumoni::JSON::String)
				return node->Text();
			if (node->GetType() == Spumoni::JSON::Object)
				return TextOf(node->Get("default"));
			return std::string();
		}

		double PanelControl::NumberOf(const Spumoni::JSON *node, double fallback)
		{
			if (node && node->GetType() == Spumoni::JSON::Number)
				return std::atof(node->Text().c_str());
			return fallback;
		}

		unsigned PanelControl::GUIColour() const
		{
			return m_Panel.Source() ? m_Panel.Source()->Colour() : 179;
		}

		PanelControl::PanelControl(Panel &panel, const Spumoni::JSON &node):
			m_Panel(panel),
			m_X(0),
			m_Y(0),
			m_W(0),
			m_H(0),
			m_Min(0),
			m_Max(1),
			m_Step(0),
			m_HasRange(false),
			m_Offset(0),
			m_Scale(1),
			m_MinProperty(NULL),
			m_MaxProperty(NULL),
			m_ScalePower(NULL),
			m_View(NULL),
			m_Property(NULL),
			m_Mapping(Direct),
			m_LogBase(10),
			m_EnabledBound(false),
			m_VisibleBound(false),
			m_ValueBound(false),
			m_Toggle(false),
			m_Node(&node),
			m_Binding(NULL),
			m_DrivenOn(false)
		{
			m_Name = TextOf(node.Get("name"));
			m_Tooltip = TextOf(node.Get("tooltip"));

			const Spumoni::JSON *bounds = node.Get("bounds");
			if (bounds && bounds->GetType() == Spumoni::JSON::Array && bounds->Size() == 4)
			{
				m_X = (int)NumberOf(bounds->At(0), 0);
				m_Y = (int)NumberOf(bounds->At(1), 0);
				m_W = (int)NumberOf(bounds->At(2), 0);
				m_H = (int)NumberOf(bounds->At(3), 0);
			}

			const Spumoni::JSON *property = node.Get("property");
			const Spumoni::JSON *source = node.Get("source");
			if (source && source->GetType() == Spumoni::JSON::String)
				m_RowSource = source->Text();
			if (property && property->GetType() == Spumoni::JSON::String)
			{
				// On a row source the property is a field of its rows; the
				// control starts on the first row's own property, for its
				// range and kind, and the panel moves it with the cursor.
				if (!m_RowSource.empty() && panel.Rows(m_RowSource))
				{
					char number[32];

					snprintf(number, sizeof(number), "%ld", panel.FirstRow(m_RowSource));
					m_RowField = property->Text();
					m_Property = panel.FindProperty(m_RowField + number);
				}
				else
					m_Property = panel.FindProperty(property->Text());
			}

			if (m_Property && m_Property->HasRange())
			{
				m_Min = m_Property->Min();
				m_Max = m_Property->Max();
				m_Step = m_Property->Step();
				m_HasRange = true;
			}
			if (node.Get("min") || node.Get("max"))
				m_HasRange = true;
			const Spumoni::JSON *min = node.Get("min"), *max = node.Get("max");
			if (min && min->GetType() == Spumoni::JSON::Object)
				m_MinProperty = panel.FindProperty(TextOf(min->Get("property")));
			else
				m_Min = NumberOf(min, m_Min);
			if (max && max->GetType() == Spumoni::JSON::Object)
				m_MaxProperty = panel.FindProperty(TextOf(max->Get("property")));
			else
				m_Max = NumberOf(max, m_Max);
			m_Step = NumberOf(node.Get("step"), m_Step);

			m_Offset = 0;
			m_Scale = 1;
			const Spumoni::JSON *mapping = node.Get("mapping");
			if (mapping && mapping->GetType() == Spumoni::JSON::Object)
			{
				m_Offset = NumberOf(mapping->Get("offset"), 0);
				m_Scale = NumberOf(mapping->Get("scale"), 1);
				if (m_Scale == 0)
					m_Scale = 1;
				if (mapping->Get("pow2"))
					m_ScalePower = panel.FindProperty(TextOf(mapping->Get("pow2")));
			}
			const std::string named = mapping && mapping->GetType() == Spumoni::JSON::Object ? TextOf(mapping->Get("kind")) : TextOf(mapping);
			if (named == "percent")
				m_Mapping = Percent;
			else if (named == "reciprocal")
				m_Mapping = Reciprocal;
			else if (named == "sqrt")
				m_Mapping = SquareRoot;
			else if (named == "symmetric_log")
			{
				m_Mapping = SymmetricLog;
				m_LogBase = NumberOf(mapping->Get("base"), 10);
				if (m_LogBase <= 1)
					m_LogBase = 10;
			}

			m_Toggle = node.Get("toggle") && node.Get("toggle")->GetType() == Spumoni::JSON::Boolean &&
				node.Get("toggle")->AsBool();
			const std::string owner = m_Name.empty() ? TextOf(node.Get("type")) : m_Name;
			m_EnabledBound = RuleOf(panel, node.Get("enabled"), "enabled", owner, m_Enabled);
			m_VisibleBound = RuleOf(panel, node.Get("visible"), "visible", owner, m_Visible);
			const Spumoni::JSON *value = node.Get("value");
			m_ValueBound = RuleOf(panel, Spiral::Data::ValueRule::Looks(value) ? value : NULL, "value", owner, m_ValueRule);
		}

		bool PanelControl::RuleOf(Panel &panel, const Spumoni::JSON *node, const char *attribute, const std::string &owner,
					  Spiral::Data::ValueRule &out)
		{
			if (!node)
				return false;
			std::string problem;
			const Panel::Scope scope(panel);
			const bool text = std::string(attribute) == "text";
			if (!Spiral::Data::ValueRule::Parse(node, scope, out, problem))
				;
			else if (text && !out.IsText())
				problem = "text needs on and off";
			else if (!text && out.IsText())
				problem = "a text is not a condition";
			else
				return true;
			panel.Report(owner + "." + attribute + ": " + problem);
			out = Spiral::Data::ValueRule();
			return false;
		}

		void PanelControl::VerifyBindings()
		{
			const Panel::Scope scope(m_Panel);
			std::string problem;
			const Spiral::Data::ValueRule *rules[] = {&m_Enabled, &m_Visible, &m_ValueRule};
			const char *names[] = {"enabled", "visible", "value"};
			bool *bound[] = {&m_EnabledBound, &m_VisibleBound, &m_ValueBound};
			for (size_t i = 0; i < 3; ++i)
				if (*bound[i] && !rules[i]->Verify(scope, problem))
				{
					m_Panel.Report(m_Name + "." + names[i] + ": " + problem);
					*bound[i] = false;
				}
			const Panel::HandlerScope handlers(m_Panel, *this);
			if (!m_Handlers.Verify(handlers, problem))
			{
				m_Panel.Report(m_Name + ".on: " + problem);
				m_Handlers = Spiral::Data::Handlers();
			}
		}

		void PanelControl::Tick()
		{
			const Panel::Scope scope(m_Panel);
			if (m_ValueBound)
				SetAttribute("value", m_ValueRule.Evaluate(scope).number);
			if (m_View && m_EnabledBound)
				m_View->Enable(m_Enabled.Holds(scope));

			if (m_View && m_VisibleBound)
				m_View->Show(m_Visible.Holds(scope));
		}

		PanelControl::~PanelControl()
		{
			delete m_View;
		}

		void PanelControl::Refresh()
		{
		}

		const Spiral::Data::Attribute *PanelControl::FindAttribute(const std::string &name) const
		{
			return name == "value" ? ValueAttribute() : NULL;
		}

		const Spiral::Data::Attribute *PanelControl::ValueAttribute() const
		{
			return m_Property && m_Property->GetKind() == PanelProperty::String ? &TextValue : &NumberValue;
		}

		bool PanelControl::GetAttribute(const Spiral::Data::Attribute &attribute, ControlValue &out) const
		{
			if (&attribute != ValueAttribute())
				return false;
			out = m_Value;
			return true;
		}

		bool PanelControl::SetAttribute(const Spiral::Data::Attribute &attribute, const ControlValue &value)
		{
			if (&attribute != ValueAttribute())
				return false;
			m_Value = value;
			Load();
			return true;
		}

		void PanelControl::BindTo(Spiral::Data::Binder &binder, Spiral::Data::DataSource &source)
		{
			if (m_Property && !m_Property->IsCommand())
				m_Binding = binder.BindValue(this, &source, source.FindField(m_RowField) >= 0 ? m_RowField : m_Property->Name(),
						new PanelConverter(*this));
		}

		bool PanelControl::DependsOn(const PanelProperty &property) const
		{
			return m_ScalePower == &property || (!m_Binding && m_Property == &property);
		}

		void PanelControl::Reload()
		{
			if (m_Binding)
				m_Binding->Refresh();
			else
				Load();
		}

		bool PanelControl::Drivable() const
		{
			return m_View && m_View->Drivable();
		}

		bool PanelControl::Drive(double amount)
		{
			if (!Drivable() || !m_View->Enabled())
				return false;

			amount = amount < 0 ? 0 : amount > 1 ? 1 : amount;

			return m_View->Drive(amount, m_DrivenOn);
		}

		double PanelControl::Value() const
		{
			if (!m_Binding)
				return m_Property ? Mapped(m_Panel.Read(*m_Property)) : 0;
			return m_Value.type == ControlValue::String ? std::atof(m_Value.text.c_str()) : m_Value.number;
		}

		double PanelControl::Physical() const
		{
			if (!m_Binding)
				return m_Property ? m_Panel.Read(*m_Property) : 0;
			return Unmapped(Value());
		}

		std::string PanelControl::Text() const
		{
			if (!m_Binding)
				return m_Property ? m_Panel.ReadText(*m_Property) : std::string();
			return m_Value.type == ControlValue::String ? m_Value.text : TextOfNumber(m_Value.number);
		}

		void PanelControl::SetAttribute(const std::string &, double)
		{
		}

		bool PanelControl::GetAttribute(const std::string &, double &) const
		{
			return false;
		}

		void PanelControl::Link(double value)
		{
			const Spiral::Data::Handlers::Actions &actions = m_Handlers.For("value_changed");
			for (size_t i = 0; i < actions.size(); ++i)
				if (actions[i].verb == Spiral::Data::Handlers::Action::Set)
					if (PanelControl *target = m_Panel.FindControl(actions[i].control))
						target->SetAttribute(actions[i].attribute, value);
		}

		bool PanelControl::LinkedValue(double &value) const
		{
			const Spiral::Data::Handlers::Actions &actions = m_Handlers.For("value_changed");
			for (size_t i = 0; i < actions.size(); ++i)
				if (actions[i].verb == Spiral::Data::Handlers::Action::Set)
				{
					const PanelControl *target = m_Panel.FindControl(actions[i].control);
					return target && target->GetAttribute(actions[i].attribute, value);
				}
			return false;
		}

		void PanelControl::Adopt(ControlView *view)
		{
			delete m_View;
			m_View = view;

			if (view && !m_Tooltip.empty())
				view->Tooltip(m_Tooltip);
		}

		double PanelControl::Scale() const
		{
			return m_ScalePower ? m_Scale * std::pow(2.0, m_Panel.Read(*m_ScalePower)) : m_Scale;
		}

		double PanelControl::Mapped(double value) const
		{
			value = (value - m_Offset) * Scale();
			switch (m_Mapping)
			{
				case Percent:
					return value * 100.0;
				case Reciprocal:
					return value != 0 ? 1.0 / value : 0;
				case SquareRoot:
					return value > 0 ? std::sqrt(value) : 0;
				case SymmetricLog:
					if (std::fabs(value) > m_LogBase)
						return value > 0 ? std::log(value) / std::log(m_LogBase) : -std::log(-value) / std::log(m_LogBase);
					return value / m_LogBase;
				case Direct:
					break;
			}
			return value;
		}

		double PanelControl::Unmapped(double value) const
		{
			switch (m_Mapping)
			{
				case Percent:
					value /= 100.0;
					break;
				case Reciprocal:
					value = value != 0 ? 1.0 / value : 0;
					break;
				case SquareRoot:
					value *= value;
					break;
				case SymmetricLog:
					if (std::fabs(value) > 1)
						value = value > 0 ? std::pow(m_LogBase, value) : -std::pow(m_LogBase, -value);
					else
						value *= m_LogBase;
					break;
				case Direct:
					break;
			}
			return value / Scale() + m_Offset;
		}

		void PanelControl::Commit(double value)
		{
			if (m_Panel.Loading())
				return;

			double physical = Unmapped(value);

			if (m_Property && !m_Property->IsReadOnly())
			{
				if (!m_Binding)
				{
					Load();
					return;
				}
				m_Value = ControlValue(value);
				Changed(*ValueAttribute());
				if (!m_Binding->Wrote)
					return;
				physical = m_Panel.Read(*m_Property);
				value = Mapped(physical);
			}
			else if (m_Property && !HasHandlers() && !IsToggle())
			{
				Load();
				return;
			}

			Fired(physical, Text());
		}

		void PanelControl::CommitText(const std::string &text)
		{
			if (m_Panel.Loading() || !m_Property || m_Property->IsReadOnly())
				return;
			if (!m_Binding)
			{
				m_Panel.WriteText(*m_Property, text);
				return;
			}
			m_Value = ControlValue(text);
			Changed(*ValueAttribute());
		}

		std::string PanelControl::TextOfNumber(double value)
		{
			char text[32];
			snprintf(text, sizeof(text), "%.9g", value);
			return text;
		}

		class PanelControl::HandlerSink: public Spiral::Data::Handlers::Sink
		{
		public:
			explicit HandlerSink(PanelControl &control): m_Control(control), m_Wrote(false) {}
			bool Wrote() const { return m_Wrote; }

			void Write(const std::string &property, const Spiral::Data::ControlValue &value,
				   const std::vector<Spiral::Data::ControlValue> &arguments)
			{
				const PanelProperty *target = m_Control.m_Panel.FindProperty(property);
				if (!target)
					return;
				if (!target->IsCommand())
				{
					m_Control.m_Panel.Write(*target, Number(value));
					m_Wrote = true;
					return;
				}
				std::vector<std::string> args;
				for (size_t i = 0; i < arguments.size(); ++i)
					args.push_back(Spell(arguments[i]));
				m_Control.m_Panel.Command(Spell(value), args);
			}

			double Delta(const std::string &property, double amount, double factor, bool byFactor)
			{
				const PanelProperty *target = m_Control.m_Panel.FindProperty(property);
				if (!target)
					return 0;
				double value = byFactor ? m_Control.m_Panel.Read(*target) * factor : m_Control.m_Panel.Read(*target) + amount;
				if (m_Control.m_HasRange || target->HasRange())
				{
					const double low = m_Control.m_HasRange ? m_Control.m_Min : target->Min();
					const double high = m_Control.m_HasRange ? m_Control.m_Max : target->Max();
					value = value < low ? low : value > high ? high : value;
				}
				if (!target->IsReadOnly())
				{
					m_Control.m_Panel.Write(*target, value);
					m_Wrote = true;
				}
				return value;
			}

			void Set(const std::string &control, const std::string &attribute, const Spiral::Data::ControlValue &value)
			{
				if (PanelControl *target = m_Control.m_Panel.FindControl(control))
					target->SetAttribute(attribute, Number(value));
			}

			void Clear(const std::string &control)
			{
				if (PanelControl *indicator = m_Control.m_Panel.FindControl(control))
					indicator->SetAttribute("clear", 0);
			}

			double Attribute(const std::string &control, const std::string &attribute) const
			{
				const PanelControl *source = m_Control.m_Panel.FindControl(control);
				double value = 0;
				if (source)
					source->GetAttribute(attribute, value);
				return value;
			}

		private:
			static double Number(const Spiral::Data::ControlValue &value)
			{
				if (value.type == Spiral::Data::ControlValue::String)
					return std::atof(value.text.c_str());
				if (value.type == Spiral::Data::ControlValue::Boolean)
					return value.boolean ? 1 : 0;
				return value.number;
			}
			static std::string Spell(const Spiral::Data::ControlValue &value)
			{
				if (value.type == Spiral::Data::ControlValue::String)
					return value.text;
				if (value.type == Spiral::Data::ControlValue::Boolean)
					return value.boolean ? "1" : "0";
				return TextOfNumber(value.number);
			}
			PanelControl &m_Control;
			bool m_Wrote;
		};

		void PanelControl::DeclareEvents(std::vector<Event> &out) const
		{
			static const Event changed = {"value_changed", {"value", "text", NULL}};
			out.push_back(changed);
		}

		bool PanelControl::DeclaresEvent(const std::string &name) const
		{
			std::vector<Event> events;
			DeclareEvents(events);
			for (size_t i = 0; i < events.size(); ++i)
				if (name == events[i].name)
					return true;
			return false;
		}

		void PanelControl::ReadHandlers()
		{
			const Spumoni::JSON &node = *m_Node;
			const std::string owner = m_Name.empty() ? TextOf(node.Get("type")) : m_Name;
			const Panel::HandlerScope scope(m_Panel, *this);
			std::string problem;
			const Spumoni::JSON *on = node.Get("on");
			if (on && !Spiral::Data::Handlers::Parse(on, scope, m_Handlers, problem))
			{
				m_Panel.Report(owner + ".on: " + problem);
				m_Handlers = Spiral::Data::Handlers();
			}
		}

		void PanelControl::Fire(const std::string &event, double value, const std::string &text)
		{
			Spiral::Data::Handlers::Arguments arguments;
			arguments["value"] = Spiral::Data::ControlValue(value);
			arguments["text"] = Spiral::Data::ControlValue(text);
			if (!m_RowSource.empty())
				arguments["row"] = Spiral::Data::ControlValue((double)m_Panel.RowNumber(m_RowSource));
			Fire(event, arguments);
		}

		void PanelControl::Fire(const std::string &event, const Spiral::Data::Handlers::Arguments &arguments)
		{
			if (!m_Handlers.Has(event))
				return;
			HandlerSink sink(*this);
			m_Handlers.Fire(event, arguments, sink);
			if (sink.Wrote())
				m_Panel.Load();
		}

		void PanelControl::Fired(double physical, const std::string &text)
		{
			if (DeclaresEvent("pressed"))
				Fire(m_Toggle ? (physical != 0 ? "on" : "off") : "pressed", physical, text);
			Fire("value_changed", physical, text);
		}
	}
}
