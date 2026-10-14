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
#include "PanelValueTable.h"
#include "Panel.h"
#include "DeviceSource.h"
#include "PanelControlRegistry.h"
#include "JSON.h"
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <cstdio>
#include <cstring>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("ValueTable", &PanelValueTable::Create);

	static int IntAt(const std::vector<unsigned char> &bytes, size_t index)
	{
		int v = 0;
		if ((index + 1) * sizeof(int) <= bytes.size())
			memcpy(&v, &bytes[index * sizeof(int)], sizeof(v));
		return v;
	}

	class ValueTableView: public Fl_Widget
	{
	public:
		ValueTableView(int x, int y, int w, int h, int columns, int rowHeight, PanelValueTable &owner):
			Fl_Widget(x, y, w, h), Columns(columns), RowHeight(rowHeight), Rows(0), Pos(-1), Top(0), Colour(179), m_Owner(owner) {}

		int Visible() const { return h() / RowHeight; }

		virtual void draw()
		{
			fl_push_clip(x(), y(), w(), h());
			fl_color(FL_BACKGROUND_COLOR);
			fl_rectf(x(), y(), w(), h());
			fl_font(FL_HELVETICA, 9);
			const int label = 22;
			const int cw = (w() - label) / Columns;
			for (int r = Top; r < Rows && r < Top + Visible(); ++r)
			{
				const int ry = y() + (r - Top) * RowHeight;
				if (r % 4 == 0)
				{
					fl_color(Colour);
					fl_rectf(x(), ry, w(), RowHeight);
				}
				char text[16];
				snprintf(text, sizeof(text), "%d", r);
				fl_color(r == Pos ? FL_RED : FL_FOREGROUND_COLOR);
				fl_draw(text, x() + 2, ry, label - 4, RowHeight, FL_ALIGN_LEFT);
				for (int c = 0; c < Columns; ++c)
				{
					fl_draw_box(FL_DOWN_BOX, x() + label + c * cw, ry + 1, cw - 1, RowHeight - 2, FL_WHITE);
					snprintf(text, sizeof(text), "%d", IntAt(Values, (size_t)r * Columns + c));
					fl_color(FL_FOREGROUND_COLOR);
					fl_draw(text, x() + label + c * cw, ry, cw - 1, RowHeight, FL_ALIGN_CENTER);
				}
			}
			fl_pop_clip();
		}

		virtual int handle(int event)
		{
			const int label = 22;
			const int cw = (w() - label) / Columns;
			const int r = Top + (Fl::event_y() - y()) / RowHeight;
			const int c = (Fl::event_x() - x() - label) / cw;
			if (event == FL_MOUSEWHEEL && Fl::event_x() < x() + label)
			{
				Top += Fl::event_dy();
				if (Top > Rows - Visible()) Top = Rows - Visible();
				if (Top < 0) Top = 0;
				redraw();
				return 1;
			}
			int delta = 0;
			if (event == FL_PUSH)
				delta = Fl::event_button() == FL_RIGHT_MOUSE ? -1 : 1;
			else if (event == FL_MOUSEWHEEL)
				delta = -Fl::event_dy();
			else
				return event == FL_ENTER || event == FL_LEAVE ? 1 : Fl_Widget::handle(event);
			if (delta && r >= 0 && r < Rows && c >= 0 && c < Columns && Fl::event_x() >= x() + label)
			{
				const size_t i = (size_t)r * Columns + c;
				const int value = m_Owner.Clamp(r, IntAt(Values, i) + delta);
				if ((i + 1) * sizeof(int) <= Values.size())
					memcpy(&Values[i * sizeof(int)], &value, sizeof(value));
				m_Owner.Act(r, c, value);
				redraw();
			}
			return 1;
		}

		int Columns, RowHeight, Rows, Pos, Top;
		Fl_Color Colour;
		std::vector<unsigned char> Values;

	private:
		PanelValueTable &m_Owner;
	};

	PanelControl *PanelValueTable::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelValueTable(panel, node);
	}

	PanelValueTable::PanelValueTable(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_Values(TextOf(node.Get("values"))),
		m_Count(TextOf(node.Get("count"))),
		m_Pos(TextOf(node.Get("position"))),
		m_SetCommand(TextOf(node.Get("set"))),
		m_Bounded(node.Get("min") && node.Get("max")),
		m_Min((int)NumberOf(node.Get("min"), 0)),
		m_Max((int)NumberOf(node.Get("max"), 0)),
		m_FirstMin((int)NumberOf(node.Get("first_min"), NumberOf(node.Get("min"), 0)))
	{
		m_View = new ValueTableView(m_X, m_Y, m_W, m_H, (int)NumberOf(node.Get("columns"), 8),
			(int)NumberOf(node.Get("row_height"), 14), *this);
		m_View->Rows = (int)NumberOf(node.Get("rows"), 0);
		m_View->Colour = (Fl_Color)GUIColour();
		Adopt(m_View);
	}

	void PanelValueTable::Load()
	{
		std::vector<unsigned char> values, count, pos;
		bool changed = false;
		if (m_Panel.Source()->ReadBytes(m_Values, values) && values != m_View->Values)
		{
			m_View->Values = values;
			changed = true;
		}
		if (m_Panel.Source()->ReadBytes(m_Count, count) && IntAt(count, 0) != m_View->Rows)
		{
			m_View->Rows = IntAt(count, 0);
			if (m_View->Top > m_View->Rows - m_View->Visible())
				m_View->Top = m_View->Rows > m_View->Visible() ? m_View->Rows - m_View->Visible() : 0;
			changed = true;
		}
		if (m_Panel.Source()->ReadBytes(m_Pos, pos) && IntAt(pos, 0) != m_View->Pos)
		{
			m_View->Pos = IntAt(pos, 0);
			changed = true;
		}
		if (changed)
			m_View->redraw();
	}

	void PanelValueTable::Refresh()
	{
		Load();
	}

	int PanelValueTable::Clamp(int row, int value) const
	{
		if (!m_Bounded)
			return value;
		const int least = row == 0 ? m_FirstMin : m_Min;
		return value < least ? least : (value > m_Max ? m_Max : value);
	}

	void PanelValueTable::Act(int row, int column, int value)
	{
		std::vector<std::string> args;
		args.push_back(TextOfNumber(row));
		args.push_back(TextOfNumber(column));
		args.push_back(TextOfNumber(value));
		m_Panel.Command(m_SetCommand, args);
	}
}
