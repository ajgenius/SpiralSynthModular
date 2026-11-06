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
#include "Input.h"
#include "FltkView.h"
#include "FltkTheme.h"
#include "JSON.h"
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <cstdio>
#include <cstring>
#include <vector>

namespace Spiral
{
	namespace Panels
	{
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
			ValueTableView(int x, int y, int w, int h, int columns, int rowHeight, ValueTableControl &owner,
					const FltkLook &look):
				Fl_Widget(x, y, w, h), Columns(columns), RowHeight(rowHeight), Rows(0), Pos(-1), Top(0), m_Owner(owner),
				m_Look(look) {}

			int Visible() const { return h() / RowHeight; }

			virtual void draw()
			{
				fl_push_clip(x(), y(), w(), h());
				fl_color(m_Look.Colour("face"));
				fl_rectf(x(), y(), w(), h());
				fl_font(m_Look.Font("text_font"), m_Look.Size("text_size"));
				const int label = 22;
				const int cw = (w() - label) / Columns;
				for (int r = Top; r < Rows && r < Top + Visible(); ++r)
				{
					const int ry = y() + (r - Top) * RowHeight;
					if (r % 4 == 0)
					{
						fl_color(m_Look.Colour("beat"));
						fl_rectf(x(), ry, w(), RowHeight);
					}
					char text[16];
					snprintf(text, sizeof(text), "%d", r);
					fl_color(m_Look.Colour(r == Pos ? "position" : "text"));
					fl_draw(text, x() + 2, ry, label - 4, RowHeight, FL_ALIGN_LEFT);
					for (int c = 0; c < Columns; ++c)
					{
						fl_draw_box(m_Look.Box("cell_box"), x() + label + c * cw, ry + 1, cw - 1, RowHeight - 2,
							m_Look.Colour("cell"));
						snprintf(text, sizeof(text), "%d", IntAt(Values, (size_t)r * Columns + c));
						fl_color(m_Look.Colour("text"));
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
			std::vector<unsigned char> Values;

		private:
			ValueTableControl &m_Owner;
			FltkLook m_Look;
		};

		namespace
		{
			class FltkTable: public FltkView, public virtual TableView
			{
			public:
				explicit FltkTable(ValueTableView *view): FltkView(view), m_View(view) {}

				void SetValues(const std::vector<unsigned char> &values)
				{
					if (values == m_View->Values)
						return;

					m_View->Values = values;
					m_View->redraw();
				}

				void SetRows(int rows)
				{
					if (rows == m_View->Rows)
						return;

					m_View->Rows = rows;

					if (m_View->Top > m_View->Rows - m_View->Visible())
						m_View->Top = m_View->Rows > m_View->Visible() ? m_View->Rows - m_View->Visible() : 0;

					m_View->redraw();
				}

				void SetPosition(int position)
				{
					if (position == m_View->Pos)
						return;

					m_View->Pos = position;
					m_View->redraw();
				}

			private:
				ValueTableView *m_View;
			};

			ControlView *MakeValueTable(PanelControl &control, const Spumoni::JSON &node)
			{
				ValueTableView *view = new ValueTableView(control.X(), control.Y(), control.W(), control.H(),
					(int)PanelControl::NumberOf(node.Get("columns"), 8), (int)PanelControl::NumberOf(node.Get("row_height"), 14),
					static_cast<ValueTableControl &>(control), FltkTheme::For(control, "ValueTable", node));
				view->Rows = (int)PanelControl::NumberOf(node.Get("rows"), 0);

				return new FltkTable(view);
			}

			ViewRegistry::Entry s_ValueTable("ValueTable", &MakeValueTable);
		}
	}
}
