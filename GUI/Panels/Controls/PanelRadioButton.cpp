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
#include "PanelRadioButton.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include "Fl_LED_Button.H"
#include <FL/Fl_Pixmap.H>
#include "WS_Sine.h"
#include "WS_Tri.h"
#include "WS_Square.h"
#include "WS_Saw.h"
#include "WS_RevSaw.h"
#include "WS_Pulse1.h"
#include "WS_Pulse2.h"
#include "WS_InvSine.h"
#include "WS_Noise.h"

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("RadioButton", &PanelRadioButton::Create);

	static unsigned char **GlyphImage(const std::string &glyph)
	{
		if (glyph == "sine") return image_Sine;
		if (glyph == "triangle") return image_Tri;
		if (glyph == "square") return image_Square;
		if (glyph == "saw") return image_Saw;
		if (glyph == "reverse_saw") return image_RevSaw;
		if (glyph == "pulse1") return image_Pulse1;
		if (glyph == "pulse2") return image_Pulse2;
		if (glyph == "inverse_sine") return image_InvSine;
		if (glyph == "noise") return image_Noise;
		return NULL;
	}

	PanelControl *PanelRadioButton::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelRadioButton(panel, node);
	}

	PanelRadioButton::PanelRadioButton(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_Glyph(NULL),
		m_HasChoice(node.Get("choice") != NULL),
		m_Choice(NumberOf(node.Get("choice"), 0))
	{
		m_Button = new Fl_LED_Button(m_X, m_Y, m_W, m_H);
		m_Button->type(FL_RADIO_BUTTON);
		if (unsigned char **image = GlyphImage(TextOf(node.Get("glyph"))))
		{
			m_Glyph = new Fl_Pixmap(image);
			m_Glyph->label(m_Button);
		}
		else
		{
			m_Button->copy_label(TextOf(node.Get("text")).c_str());
			m_Button->labelsize(10);
			m_Button->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
			m_Button->down_box(FL_ROUND_DOWN_BOX);
		}
		m_Button->callback(Changed, this);
		Adopt(m_Button);
	}

	PanelRadioButton::~PanelRadioButton()
	{
		delete m_Glyph;
	}

	void PanelRadioButton::Load()
	{
		if (!Property())
			return;
		const double value = Physical();
		m_Button->value(m_HasChoice ? value == m_Choice : value != 0);
	}

	void PanelRadioButton::Changed(Fl_Widget *, void *data)
	{
		PanelRadioButton *self = (PanelRadioButton *)data;
		if (self->m_HasChoice)
		{
			if (self->m_Button->value())
				self->Commit(self->m_Choice);
		}
		else
			self->Commit(self->m_Button->value() ? 1 : 0);
	}
}
