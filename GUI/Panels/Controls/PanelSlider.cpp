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
#include "PanelSlider.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include <FL/Fl_Slider.H>
#include <cstdio>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("Slider", &PanelSlider::Create);

	PanelControl *PanelSlider::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelSlider(panel, node);
	}

	PanelSlider::PanelSlider(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_ValueLabel(TextOf(node.Get("value_label"))),
		m_StepFraction(NumberOf(node.Get("step_fraction"), 0))
	{
		const Spumoni::JSON *vertical = node.Get("vertical");
		const Spumoni::JSON *inverted = node.Get("inverted");
		const bool isVertical = vertical ? vertical->AsBool() : m_H > m_W;
		const bool isInverted = inverted && inverted->AsBool();

		m_Slider = new Fl_Slider(m_X, m_Y, m_W, m_H);
		if (TextOf(node.Get("style")) == "plain")
			m_Slider->type(isVertical ? FL_VERTICAL : FL_HORIZONTAL);
		else
			m_Slider->type(isVertical ? FL_VERT_NICE_SLIDER : FL_HOR_NICE_SLIDER);
		m_Slider->box(FL_PLASTIC_DOWN_BOX);
		m_Slider->selection_color((Fl_Color)GUIColour());
		m_Flip = isVertical ? !isInverted : isInverted;
		if (m_Flip)
			m_Slider->range(Mapped(m_Max), Mapped(m_Min));
		else
			m_Slider->range(Mapped(m_Min), Mapped(m_Max));
		if (m_Step > 0)
			m_Slider->step(m_Step);
		if (!m_ValueLabel.empty())
		{
			m_Slider->labelsize(10);
			m_Slider->labelcolor((Fl_Color)GUIColour());
		}
		m_Slider->callback(Changed, this);
		Adopt(m_Slider);
	}

	void PanelSlider::Load()
	{
		if (m_MinProperty || m_MaxProperty)
		{
			double low = m_Flip ? m_Slider->maximum() : m_Slider->minimum();
			double high = m_Flip ? m_Slider->minimum() : m_Slider->maximum();
			if (m_MinProperty)
				low = Mapped(m_Panel.Read(*m_MinProperty));
			if (m_MaxProperty)
				high = Mapped(m_Panel.Read(*m_MaxProperty));
			if (low < high)
			{
				m_Slider->range(m_Flip ? high : low, m_Flip ? low : high);
				// A step that is a share of the range, as the old editors stepped their
				// own 0..1 slider and scaled it to the limits.
				if (m_StepFraction > 0)
					m_Slider->step(m_StepFraction * (high - low));
			}
		}
		if (Property())
		{
			m_Slider->value(Value());
			ShowValue(Physical());
		}
	}

	void PanelSlider::Refresh()
	{
		if (m_MaxProperty || (Property() && Property()->IsReadOnly()))
			Load();
	}

	void PanelSlider::ShowValue(double value)
	{
		if (m_ValueLabel.empty())
			return;
		char text[64];
		std::snprintf(text, sizeof(text), ("    " + m_ValueLabel + "   ").c_str(), value);
		m_Slider->align(FL_ALIGN_INSIDE | (value < 0 ? FL_ALIGN_LEFT : FL_ALIGN_RIGHT));
		m_Slider->copy_label(text);
	}

	void PanelSlider::Changed(Fl_Widget *, void *data)
	{
		PanelSlider *self = (PanelSlider *)data;
		self->Commit(self->m_Slider->value());
		self->ShowValue(self->Unmapped(self->m_Slider->value()));
	}
}
