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
#include "PanelMatrixGrid.h"
#include "Panel.h"
#include "DeviceSource.h"
#include "PanelControlRegistry.h"
#include "JSON.h"
#include "Fl_DamageArea.H"
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Slider.H>
#include <FL/fl_draw.H>
#include <algorithm>
#include <climits>
#include <cstring>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("MatrixGrid", &PanelMatrixGrid::Create);

	static const char *NoteText[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

	class MatrixGridView: public Fl_Widget
	{
	public:
		MatrixGridView(int x, int y, int w, int h, int columns, int rows, PanelMatrixGrid &owner):
			Fl_Widget(x, y, w, h), Columns(columns), Rows(rows), Step(-1),
			NoteNames(0), BaseH(h), Classic(true), TriggerRow(INT_MIN), m_Marking(false),
			m_DragC(-1), m_DragR(0), m_DragY(0), m_DragV(1.0f),
			m_EndC(-1), m_EndR(0), m_HoverC(-1), m_HoverR(0),
			m_Slider(NULL), m_SliderC(0), m_SliderR(0), m_Owner(owner) {}

		~MatrixGridView() { CloseSlider(); }

		int Gutter() const { return BaseH > 0 ? NoteNames * h() / BaseH : NoteNames; }
		int CellsX() const { return x() + Gutter(); }
		int CellsY() const { return y(); }
		int CellsW() const { return w() - Gutter(); }
		int CellsH() const { return h(); }

		float VolumeAt(int c, int r) const
		{
			const size_t i = ((size_t)c * Rows + r) * sizeof(float);
			float v = 1.0f;
			if (i + sizeof(float) <= Volumes.size())
				memcpy(&v, &Volumes[i], sizeof(v));
			return v;
		}

		bool At(int c, int r) const
		{
			const size_t i = (size_t)c * Rows + r;
			return i < Cells.size() && Cells[i];
		}

		// Where the note starting at c on row r stops, as the private
		// tree's grid has it: its own end, the next note on the row, or the
		// grid's edge. A device that sends no ends has one-step notes.
		int EndOf(int c, int r) const
		{
			const size_t i = (size_t)c * Rows + r;
			const int end = i < Ends.size() ? std::max(c + 1, std::min(Columns, (int)Ends[i])) : c + 1;
			for (int next = c + 1; next < end; ++next)
				if (At(next, r))
					return next;
			return end;
		}

		// The start of the note covering cell c on row r, or -1.
		int StartOf(int c, int r) const
		{
			for (int s = c; s >= 0; --s)
				if (At(s, r))
					return EndOf(s, r) > c ? s : -1;
			return -1;
		}

		int ColumnX(int c) const { return CellsX() + c * CellsW() / Columns; }
		int RowY(int r) const { return CellsY() + (Rows - 1 - r) * CellsH() / Rows; }

		// The cells of a note, or what it covered, drawn again.
		void DamageNote(int c0, int c1, int r)
		{
			if (c0 < 0 || r < 0 || r >= Rows || !visible_r())
				return;
			c1 = std::min(Columns, std::max(c0 + 1, c1));
			const int ry = RowY(r), rh = CellsY() + (Rows - r) * CellsH() / Rows - ry;
			Fl_DamageArea(this, ColumnX(c0) - 1, ry - 1, ColumnX(c1) - ColumnX(c0) + 3, rh + 3);
		}

		static bool BlackKey(int r)
		{
			const int n = r % 12;
			return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
		}

		Fl_Color CellColour(int c, int r) const
		{
			const int start = StartOf(c, r);
			if (start >= 0)
				c = start;
			const float volume = std::max(0.0f, std::min(1.0f, VolumeAt(c, r)));
			if (Classic)
			{
				if (start >= 0)
				{
					const int v = Volumes.empty() ? 255 : (int)(255 * volume);
					return fl_rgb_color(v, v, 255);
				}
				if (c % 8 == 0) return fl_rgb_color(150, 150, 150);
				if (BlackKey(r)) return fl_rgb_color(170, 170, 170);
				return FL_GRAY;
			}
			const int fade = (int)(200 * (1.0f - volume));
			if (start >= 0)
				return Volumes.empty() ? (Fl_Color)179 : fl_rgb_color(fade, fade, 255);
			return c == Step ? FL_GRAY : (c % 4 ? FL_WHITE : fl_rgb_color(225, 225, 225));
		}

		// The playing step: only the column it left and the one it reached
		// are drawn again, and the classic grid does not show it at all.
		void SetStep(int s)
		{
			if (s == Step)
				return;
			const int was = Step;
			Step = s;
			if (Classic)
				return;
			DamageColumn(was);
			DamageColumn(s);
		}

		void DamageColumn(int c)
		{
			if (c < 0 || c >= Columns || !visible_r())
				return;
			const int cx = CellsX() + c * CellsW() / Columns, cw = CellsX() + (c + 1) * CellsW() / Columns - cx;
			Fl_DamageArea(this, cx, CellsY(), cw + 1, CellsH() + 1);
		}

		virtual void draw()
		{
			fl_color(Classic ? FL_BACKGROUND_COLOR : FL_DARK3);
			fl_rectf(x(), y(), w(), h());
			const int x0 = CellsX(), y0 = CellsY(), cw0 = CellsW(), ch0 = CellsH();
			// Only the cells in the clip: a step damages two columns.
			int kx, ky, kw, kh;
			fl_clip_box(x(), y(), w(), h(), kx, ky, kw, kh);
			for (int c = 0; c < Columns; ++c)
				for (int r = 0; r < Rows; ++r)
				{
					const int cx = x0 + c * cw0 / Columns, cw = x0 + (c + 1) * cw0 / Columns - cx;
					const int ry = y0 + (Rows - 1 - r) * ch0 / Rows, rh = y0 + (Rows - r) * ch0 / Rows - ry;
					if (cx > kx + kw || cx + cw < kx || ry > ky + kh || ry + rh < ky)
						continue;
					const Fl_Color colour = CellColour(c, r);
					// A note longer than a step is one bar: no line inside it.
					const int start = StartOf(c, r);
					const bool joined = start >= 0 && start < c;
					fl_color(colour);
					if (Classic)
					{
						fl_rectf(cx, ry, cw, rh);
						fl_color(FL_BLACK);
						fl_rect(cx, ry, cw + 1, rh + 1);
						if (joined)
						{
							fl_color(colour);
							fl_rectf(cx, ry + 1, 1, rh - 1);
						}
					}
					else
						fl_rectf(joined ? cx - 1 : cx, ry, joined ? cw : cw - 1, rh - 1);
				}
			// The note under the pointer, outlined, with the handle that
			// drags its end at its right edge.
			const int hc = m_EndC >= 0 ? m_EndC : m_HoverC, hr = m_EndC >= 0 ? m_EndR : m_HoverR;
			if (hc >= 0 && At(hc, hr))
			{
				const int left = ColumnX(hc), right = ColumnX(EndOf(hc, hr));
				const int ry = RowY(hr), rh = CellsY() + (Rows - hr) * CellsH() / Rows - ry;
				fl_color(FL_SELECTION_COLOR);
				fl_rect(left, ry, right - left + 1, rh + 1);
				fl_rectf(right - 2, ry + 1, 2, rh - 1);
			}
			if (NoteNames > 0)
			{
				fl_font(FL_HELVETICA, 8);
				for (int r = 0; r < Rows; ++r)
				{
					const int ry = y0 + (Rows - 1 - r) * ch0 / Rows, rh = y0 + (Rows - r) * ch0 / Rows - ry;
					// The trigger rows behind their names, Trigger 1's the
					// strongest.
					if (TriggerRow != INT_MIN && r >= TriggerRow && r < TriggerRow + Triggers)
					{
						fl_color(fl_color_average(FL_SELECTION_COLOR, FL_BACKGROUND_COLOR, r == TriggerRow ? 0.7f : 0.3f));
						fl_rectf(x(), ry, Gutter(), rh);
					}
					fl_color(FL_FOREGROUND_COLOR);
					fl_draw(NoteText[r % 12], x() + 2, ry, Gutter() - 2, rh, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
				}
			}
		}

		// The rows sending Triggers 1 to 16, from the row given, which may
		// be off the grid.
		void SetTriggerRow(int row)
		{
			if (row == TriggerRow)
				return;
			TriggerRow = row;
			if (NoteNames > 0 && visible_r())
				Fl_DamageArea(this, x(), y(), Gutter() + 1, h() + 1);
		}

		// The note names mark the trigger rows: a click or a drag there
		// moves Trigger 1 to the row under the pointer.
		bool InNames() const
		{
			return NoteNames > 0 && Fl::event_x() < CellsX() && Fl::event_y() >= CellsY();
		}

		void MarkTriggers()
		{
			const int r = std::max(0, std::min(Rows - 1, Rows - 1 - (Fl::event_y() - CellsY()) * Rows / std::max(1, CellsH())));
			if (r == TriggerRow)
				return;
			SetTriggerRow(r);
			m_Owner.MoveTriggers(r);
		}

		bool Cell(int &c, int &r) const
		{
			if (Fl::event_x() < CellsX() || Fl::event_y() < CellsY())
				return false;
			c = (Fl::event_x() - CellsX()) * Columns / CellsW();
			r = Rows - 1 - (Fl::event_y() - CellsY()) * Rows / CellsH();
			return c >= 0 && c < Columns && r >= 0 && r < Rows;
		}

		void SetVolume(int c, int r, float v)
		{
			v = std::max(0.0f, std::min(1.0f, v));
			const size_t i = ((size_t)c * Rows + r) * sizeof(float);
			if (i + sizeof(float) <= Volumes.size())
				memcpy(&Volumes[i], &v, sizeof(v));
			m_Owner.Volume(c, r, v);
			redraw();
		}

		void OpenSlider(int c, int r)
		{
			CloseSlider();
			const int cx = CellsX() + c * CellsW() / Columns, cw = CellsX() + (c + 1) * CellsW() / Columns - cx;
			const int ry = CellsY() + (Rows - 1 - r) * CellsH() / Rows, rh = CellsY() + (Rows - r) * CellsH() / Rows - ry;
			m_Slider = new Fl_Slider(cx, ry + rh, cw + 1, 50);
			m_Slider->type(FL_VERT_NICE_SLIDER);
			m_Slider->selection_color((Fl_Color)179);
			m_Slider->maximum(255);
			m_Slider->step(1);
			m_Slider->value(255 - (int)(255 * VolumeAt(c, r)));
			m_Slider->callback(SliderChanged, this);
			m_SliderC = c; m_SliderR = r;
			parent()->add(m_Slider);
			parent()->redraw();
		}

		void CloseSlider()
		{
			if (!m_Slider)
				return;
			if (parent())
			{
				parent()->remove(m_Slider);
				parent()->redraw();
			}
			Fl::delete_widget(m_Slider);
			m_Slider = NULL;
		}

		static void SliderChanged(Fl_Widget *w, void *data)
		{
			MatrixGridView *self = (MatrixGridView *)data;
			self->SetVolume(self->m_SliderC, self->m_SliderR, (float)((255 - ((Fl_Slider *)w)->value()) / 255.0));
		}

		void Hover(int c, int r)
		{
			if (c == m_HoverC && (c < 0 || r == m_HoverR))
				return;
			if (m_HoverC >= 0)
				DamageNote(m_HoverC, EndOf(m_HoverC, m_HoverR), m_HoverR);
			m_HoverC = c; m_HoverR = r;
			if (c >= 0)
				DamageNote(c, EndOf(c, r), r);
		}

		// The end follows the pointer, a step past the cell it is over,
		// up to the next note on the row.
		void DragEnd()
		{
			const int column = (Fl::event_x() - CellsX()) * Columns / std::max(1, CellsW()) + 1;
			int end = std::max(m_EndC + 1, std::min(Columns, column));
			for (int next = m_EndC + 1; next < end; ++next)
				if (At(next, m_EndR))
				{
					end = next;
					break;
				}
			const int was = EndOf(m_EndC, m_EndR);
			if (end == was)
				return;
			const size_t i = (size_t)m_EndC * Rows + m_EndR;
			if (i < Ends.size())
				Ends[i] = (unsigned char)end;
			m_Owner.Resize(m_EndC, m_EndR, end);
			DamageNote(m_EndC, std::max(was, end), m_EndR);
		}

		virtual int handle(int event)
		{
			int c, r;
			switch (event)
			{
				case FL_PUSH:
				{
					if (m_Slider && Fl::event_button() == FL_LEFT_MOUSE)
						CloseSlider();
					if (Fl::event_button() == FL_LEFT_MOUSE && InNames() && m_Owner.HasTriggers())
					{
						m_Marking = true;
						MarkTriggers();
						return 1;
					}
					if (!Cell(c, r))
						return 1;
					if (Fl::event_button() == FL_RIGHT_MOUSE)
					{
						if (m_Slider)
						{
							CloseSlider();
							return 1;
						}
						if (StartOf(c, r) < 0 || !m_Owner.HasVolume())
							return 1;
						c = StartOf(c, r);
						OpenSlider(c, r);
						m_DragC = c; m_DragR = r; m_DragY = Fl::event_y();
						m_DragV = VolumeAt(c, r);
						return 1;
					}
					m_DragC = -1;
					const int start = StartOf(c, r);
					if (start >= 0 && m_Owner.HasResize() &&
					    Fl::event_x() >= ColumnX(EndOf(start, r)) - std::max(2, std::min(3, CellsW() / Columns / 4)))
					{
						// The right edge of a note: drag its end.
						m_EndC = start; m_EndR = r;
						DamageNote(start, EndOf(start, r), r);
						return 1;
					}
					// A click inside a note takes it away; one on an empty
					// cell puts a note there, and a drag right lengthens it.
					const int at = start >= 0 ? start : c;
					const bool on = start < 0;
					const size_t i = (size_t)at * Rows + r;
					const int was = start >= 0 ? EndOf(start, r) : c + 1;
					if (i < Cells.size())
						Cells[i] = on;
					if (i < Ends.size())
						Ends[i] = (unsigned char)(at + 1);
					m_Owner.Act(at, r, on ? 1 : 0);
					if (on && m_Owner.HasResize())
					{
						m_EndC = at; m_EndR = r;
					}
					DamageNote(at, std::max(was, EndOf(at, r)), r);
					return 1;
				}
				case FL_DRAG:
					if (m_Marking)
					{
						MarkTriggers();
						return 1;
					}
					if (m_EndC >= 0)
					{
						DragEnd();
						return 1;
					}
					if (m_DragC >= 0)
					{
						SetVolume(m_DragC, m_DragR, m_DragV + (m_DragY - Fl::event_y()) / 100.0f);
						if (m_Slider)
							m_Slider->value(255 - (int)(255 * VolumeAt(m_DragC, m_DragR)));
					}
					return 1;
				case FL_RELEASE:
					m_Marking = false;
					m_DragC = -1;
					if (m_EndC >= 0)
					{
						const int c0 = m_EndC, r0 = m_EndR;
						m_EndC = -1;
						DamageNote(c0, Columns, r0);
					}
					return 1;
				case FL_ENTER:
				case FL_MOVE:
				{
					const bool inside = Cell(c, r);
					Hover(inside ? StartOf(c, r) : -1, r);
					return 1;
				}
				case FL_LEAVE:
					Hover(-1, 0);
					return 1;
				case FL_MOUSEWHEEL:
					if (Cell(c, r) && StartOf(c, r) >= 0 && m_Owner.HasVolume())
					{
						c = StartOf(c, r);
						SetVolume(c, r, VolumeAt(c, r) - Fl::event_dy() * 0.05f);
						return 1;
					}
					return 0;
			}
			return Fl_Widget::handle(event);
		}

		int Columns, Rows, Step;
		int NoteNames, BaseH;
		bool Classic;
		enum { Triggers = 16 };
		int TriggerRow;         // Trigger 1's row, or INT_MIN: none shown
		bool m_Marking;         // dragging the trigger rows
		std::vector<unsigned char> Cells, Volumes, Ends;
		int m_DragC, m_DragR, m_DragY;
		float m_DragV;
		int m_EndC, m_EndR;     // the note whose end is being dragged
		int m_HoverC, m_HoverR; // the note under the pointer
		Fl_Slider *m_Slider;
		int m_SliderC, m_SliderR;

	private:
		PanelMatrixGrid &m_Owner;
	};

	PanelControl *PanelMatrixGrid::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelMatrixGrid(panel, node);
	}

	PanelMatrixGrid::PanelMatrixGrid(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_Channel(TextOf(node.Get("grid"))),
		m_StepChannel(TextOf(node.Get("step"))),
		m_VolumeChannel(TextOf(node.Get("volume"))),
		m_EndChannel(TextOf(node.Get("end"))),
		m_TriggerChannel(TextOf(node.Get("triggers")))
	{
		m_View = new MatrixGridView(m_X, m_Y, m_W, m_H, (int)NumberOf(node.Get("columns"), 16),
			(int)NumberOf(node.Get("rows"), 8), *this);
		m_View->NoteNames = (int)NumberOf(node.Get("note_names"), 0);
		m_View->Classic = TextOf(node.Get("scheme")) != "fade";
		Adopt(m_View);
	}

	void PanelMatrixGrid::Load()
	{
		std::vector<unsigned char> cells;
		if (m_Panel.Source()->ReadBytes(m_Channel, cells) && cells != m_View->Cells)
		{
			m_View->Cells = cells;
			m_View->redraw();
		}
		std::vector<unsigned char> volumes;
		if (m_View->m_DragC < 0 && m_Panel.Source()->ReadBytes(m_VolumeChannel, volumes) && volumes != m_View->Volumes)
		{
			m_View->Volumes = volumes;
			m_View->redraw();
		}
		std::vector<unsigned char> ends;
		if (!m_EndChannel.empty() && m_View->m_EndC < 0 && m_Panel.Source()->ReadBytes(m_EndChannel, ends) && ends != m_View->Ends)
		{
			m_View->Ends = ends;
			m_View->redraw();
		}
	}

	void PanelMatrixGrid::Refresh()
	{
		Load();
		std::vector<unsigned char> step;
		if (m_Panel.Source()->ReadBytes(m_StepChannel, step) && step.size() >= sizeof(int))
		{
			int s;
			memcpy(&s, &step[0], sizeof(s));
			m_View->SetStep(s);
		}
		std::vector<unsigned char> row;
		if (!m_TriggerChannel.empty() && !m_View->m_Marking && m_Panel.Source()->ReadBytes(m_TriggerChannel, row) &&
		    row.size() >= sizeof(int))
		{
			int r;
			memcpy(&r, &row[0], sizeof(r));
			m_View->SetTriggerRow(r);
		}
	}

	void PanelMatrixGrid::Act(int x, int y, int on)
	{
		Spiral::Data::Handlers::Arguments arguments;
		arguments["x"] = Spiral::Data::ControlValue((double)x);
		arguments["y"] = Spiral::Data::ControlValue((double)y);
		Fire(on ? "activate" : "deactivate", arguments);
	}

	bool PanelMatrixGrid::HasVolume() const
	{
		return Handles("volume");
	}

	void PanelMatrixGrid::Volume(int x, int y, float volume)
	{
		if (!HasVolume())
			return;
		Spiral::Data::Handlers::Arguments arguments;
		arguments["x"] = Spiral::Data::ControlValue((double)x);
		arguments["y"] = Spiral::Data::ControlValue((double)y);
		arguments["volume"] = Spiral::Data::ControlValue((double)volume);
		Fire("volume", arguments);
	}

	bool PanelMatrixGrid::HasResize() const
	{
		return Handles("resize");
	}

	// A note's end dragged: the step after its last.
	void PanelMatrixGrid::Resize(int x, int y, int end)
	{
		if (!HasResize())
			return;
		Spiral::Data::Handlers::Arguments arguments;
		arguments["x"] = Spiral::Data::ControlValue((double)x);
		arguments["y"] = Spiral::Data::ControlValue((double)y);
		arguments["end"] = Spiral::Data::ControlValue((double)end);
		Fire("resize", arguments);
	}

	bool PanelMatrixGrid::HasTriggers() const
	{
		return Handles("triggers");
	}

	// Trigger 1 moved to the grid's row y.
	void PanelMatrixGrid::MoveTriggers(int y)
	{
		if (!HasTriggers())
			return;
		Spiral::Data::Handlers::Arguments arguments;
		arguments["y"] = Spiral::Data::ControlValue((double)y);
		Fire("triggers", arguments);
	}

	void PanelMatrixGrid::DeclareEvents(std::vector<Event> &out) const
	{
		static const Event activate = {"activate", {"x", "y", NULL}};
		static const Event deactivate = {"deactivate", {"x", "y", NULL}};
		static const Event volume = {"volume", {"x", "y", "volume", NULL}};
		static const Event resize = {"resize", {"x", "y", "end", NULL}};
		static const Event triggers = {"triggers", {"y", NULL}};
		PanelControl::DeclareEvents(out);
		out.push_back(activate);
		out.push_back(deactivate);
		out.push_back(volume);
		out.push_back(resize);
		out.push_back(triggers);
	}
}
