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
#include "PanelSevenSegment.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include "Fl_SevenSeg.H"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <cstdio>
#include <limits>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("SevenSegment", &PanelSevenSegment::Create);

	PanelControl *PanelSevenSegment::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelSevenSegment(panel, node);
	}

	PanelSevenSegment::PanelSevenSegment(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_Time(TextOf(node.Get("format")) == "time"),
		m_Shown(std::numeric_limits<double>::quiet_NaN())
	{
		const int digits = (int)NumberOf(node.Get("digits"), 1);
		const int width = digits > 0 ? m_W / digits : m_W;
		m_Group = new Fl_Group(m_X, m_Y, m_W, m_H);
		for (int dis = 0; dis < digits; ++dis)
		{
			Fl_SevenSeg *digit = new Fl_SevenSeg(m_X + width * dis, m_Y, width, m_H);
			digit->bar_width((int)NumberOf(node.Get("bar_width"), 4));
			digit->color(fl_rgb_color(100, 200, 100));
			digit->color2(fl_rgb_color(20, 60, 20));
			if (m_Time && dis > 0 && dis % 2 == 0)
				digit->dp(colon);
			m_Digits.push_back(digit);
		}
		m_Group->end();
		Adopt(m_Group);
		Show(0);
	}

	void PanelSevenSegment::Load()
	{
		if (Property())
			Show(Value());
	}

	void PanelSevenSegment::SetAttribute(const std::string &attribute, double value)
	{
		if (attribute == "value")
			Show(value);
	}

	void PanelSevenSegment::Show(double value)
	{
		if (value == m_Shown)
			return;
		m_Shown = value;
		if (m_Time)
		{
			const float t = value > 0 ? (float)value : 0;
			const int places[6] = { (int)(t / 600) % 10, (int)(t / 60) % 10, (int)(t / 10) % 6, (int)t % 10, (int)(t * 10) % 10, (int)(t * 100) % 10 };
			for (size_t i = 0; i < m_Digits.size() && i < 6; ++i)
				m_Digits[i]->value(places[i]);
		}
		else
		{
			char text[64];
			std::snprintf(text, sizeof(text), "%1.5f", value);
			const char *c = text;
			for (size_t i = 0; i < m_Digits.size(); ++i)
			{
				const bool dot = *c == '.';
				m_Digits[i]->dp(dot ? point : off);
				if (*c == 0)
					m_Digits[i]->value(0);
				else
				{
					if (dot)
						c++;
					m_Digits[i]->value(*c == '-' ? -1 : (int)*c - (int)'0');
					c++;
				}
			}
		}
	}
}
