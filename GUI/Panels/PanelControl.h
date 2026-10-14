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
#ifndef SPIRAL_PANELS_PANEL_CONTROL_H
#define SPIRAL_PANELS_PANEL_CONTROL_H

#include "Binding.h"
#include "Handlers.h"
#include "ValueRule.h"
#include <string>
#include <vector>

class Fl_Widget;

namespace Spumoni
{
	class JSON;
}

namespace SpiralPanels
{
	class Panel;
	class PanelProperty;

	class PanelConverter;

	class PanelControl: public Spiral::Data::Bindable
	{
	public:
		virtual ~PanelControl();

		const std::string &Name() const { return m_Name; }
		Fl_Widget *Widget() const { return m_Widget; }

		virtual void Load() {}
		virtual void Refresh();
		virtual void Tick();
		// Reads the handlers once the control is whole (its events are its
		// class's), then checks the control bindings and handler targets once
		// every control of the panel exists.
		void ReadHandlers();
		void VerifyBindings();
		virtual void SetAttribute(const std::string &attribute, double value);
		virtual bool GetAttribute(const std::string &attribute, double &value) const;
		virtual const Spiral::Data::Attribute *FindAttribute(const std::string &name) const;
		virtual const Spiral::Data::Attribute *ValueAttribute() const;
		virtual bool GetAttribute(const Spiral::Data::Attribute &attribute, Spiral::Data::ControlValue &out) const;
		virtual bool SetAttribute(const Spiral::Data::Attribute &attribute, const Spiral::Data::ControlValue &value);
		void BindTo(Spiral::Data::Binder &binder, Spiral::Data::DataSource &source);
		bool Bound() const { return m_Binding != NULL; }
		bool DependsOn(const PanelProperty &property) const;
		void Reload();

		static std::string TextOf(const Spumoni::JSON *node);
		static double NumberOf(const Spumoni::JSON *node, double fallback);
		bool HasHandlers() const { return !m_Handlers.Empty(); }
		bool Handles(const std::string &event) const { return m_Handlers.Has(event); }

	protected:
		PanelControl(Panel &panel, const Spumoni::JSON &node);

		void Adopt(Fl_Widget *widget);
		unsigned GUIColour() const;
		void Commit(double value);
		void CommitText(const std::string &text);
		double Value() const;
		double Physical() const;
		std::string Text() const;
		bool LinkedValue(double &value) const;
		void Link(double value);
		double Mapped(double value) const;
		double Unmapped(double value) const;
		double Scale() const;
		const PanelProperty *Property() const { return m_Property; }
		// A toggle is stated (`toggle: true`); the older write_off implied it.
		bool IsToggle() const { return m_Toggle; }
		// An event this control declares, with the names of its arguments.
		struct Event
		{
			const char *name;
			const char *arguments[4];
		};
		// Which events this control fires. Every control changes its value;
		// a button is pressed, a toggle goes on and off, a file button
		// changes its filename, a grid activates cells.
		virtual void DeclareEvents(std::vector<Event> &out) const;
		bool DeclaresEvent(const std::string &name) const;
		// Fires an event with its value (and text) into the handlers.
		void Fire(const std::string &event, double value, const std::string &text = std::string());
		void Fire(const std::string &event, const Spiral::Data::Handlers::Arguments &arguments);
		// What a committed value fires: value_changed, and for a button its
		// press or its on/off before that.
		virtual void Fired(double physical, const std::string &text);
		static std::string TextOfNumber(double value);

		Panel &m_Panel;
		std::string m_Name;
		int m_X, m_Y, m_W, m_H;
		double m_Min, m_Max, m_Step;
		bool m_HasRange;
		const PanelProperty *m_MinProperty, *m_MaxProperty;
		double m_Offset, m_Scale;
		const PanelProperty *m_ScalePower;
		// What `enabled`, `visible` and `value` are bound to. A rule read from the
		// old spelling (enabled_when, visible_when, follows) is the same rule.
		Spiral::Data::ValueRule m_Enabled, m_Visible, m_ValueRule;
		bool m_EnabledBound, m_VisibleBound, m_ValueBound;
		// Reads a binding from `node`, reporting a refused one against the
		// panel. True when a rule was given at all.
		static bool RuleOf(Panel &panel, const Spumoni::JSON *node, const char *attribute, const std::string &owner,
				   Spiral::Data::ValueRule &out);

	private:
		enum Mapping
		{
			Direct,
			Percent,
			Reciprocal,
			SquareRoot,
			SymmetricLog
		};
		double m_LogBase;

		Fl_Widget *m_Widget;
		const PanelProperty *m_Property;
		std::string m_Tooltip;
		Mapping m_Mapping;
		// What the control does, by event: `on` in the file, and the older
		// write, write_off, delta, sets, clear_indicators and file.then read
		// into the same handlers under the event each one fired on.
		Spiral::Data::Handlers m_Handlers;
		bool m_Toggle;
		const Spumoni::JSON *m_Node;
		class HandlerSink;
		friend class HandlerSink;
		Spiral::Data::Binding *m_Binding;
		Spiral::Data::ControlValue m_Value;
		friend class PanelConverter;
		friend class Panel;
	};
}

#endif
