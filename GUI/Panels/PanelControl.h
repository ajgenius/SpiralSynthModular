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
		virtual bool Drivable() const;
		bool Drive(double amount);

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
		bool IsToggle() const { return m_Toggle; }
		struct Event
		{
			const char *name;
			const char *arguments[4];
		};
		virtual void DeclareEvents(std::vector<Event> &out) const;
		bool DeclaresEvent(const std::string &name) const;
		void Fire(const std::string &event, double value, const std::string &text = std::string());
		void Fire(const std::string &event, const Spiral::Data::Handlers::Arguments &arguments);
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
		Spiral::Data::ValueRule m_Enabled, m_Visible, m_ValueRule;
		bool m_EnabledBound, m_VisibleBound, m_ValueBound;
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
		Spiral::Data::Handlers m_Handlers;
		bool m_Toggle;
		const Spumoni::JSON *m_Node;
		class HandlerSink;
		friend class HandlerSink;
		Spiral::Data::Binding *m_Binding;
		Spiral::Data::ControlValue m_Value;
		bool m_DrivenOn;
		friend class PanelConverter;
		friend class Panel;
	};
}

#endif
