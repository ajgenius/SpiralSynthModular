// SPDX-License-Identifier: GPL-2.0-or-later
#include "Patch.h"
#include "FltkView.h"
#include "FltkTheme.h"
#include "JSON.h"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_draw.H>
#include <algorithm>
#include <cstdio>

namespace Spiral
{
	namespace Panels
	{
		namespace
		{
			// A scroll bar of the roll's. While it is held the page does not
			// turn after the playhead.
			class RollBar: public Fl_Scrollbar
			{
			public:
				RollBar(PianoRollControl &control): Fl_Scrollbar(0, 0, 1, 1), m_Control(control) {}

				int handle(int event)
				{
					if (event == FL_PUSH)
						m_Control.HoldView(true);
					else if (event == FL_RELEASE)
						m_Control.HoldView(false);

					return Fl_Scrollbar::handle(event);
				}

			private:
				PianoRollControl &m_Control;
			};

			// The roll, with its own scroll bars: time along the bottom, as
			// far as the notes go and a page beyond; keys down the side.
			class PianoRollWidget: public Fl_Group
			{
			public:
				enum
				{
					Bar = 14,
					Keys = 128
				};

				PianoRollWidget(int x, int y, int w, int h, PianoRollControl &control, const FltkLook &look):
					Fl_Group(x, y, w, h),
					m_Control(control),
					m_Look(look)
				{
					look.Apply(this);
					m_Time = new RollBar(control);
					m_Time->type(FL_HORIZONTAL);
					m_Time->callback(&TimeMoved, this);
					m_Keys = new RollBar(control);
					m_Keys->callback(&KeysMoved, this);
					end();
					PlaceBars();
				}

				// The part the roll draws in, beside its bars.
				int RollW() const { return w() - Bar; }
				int RollH() const { return h() - Bar; }

				void resize(int nx, int ny, int nw, int nh)
				{
					Fl_Widget::resize(nx, ny, nw, nh);
					PlaceBars();
				}

				void draw()
				{
					NoteGrid &grid = m_Control.Grid();
					const int left = x() + (int)grid.Gutter;
					const int top = y() + (int)grid.Header;
					const double rowHeight = grid.RowHeight();
					SyncBars(grid);
					fl_color(m_Look.Colour("face"));
					fl_rectf(x(), y(), w(), h());
					draw_children();
					fl_push_clip(x(), y(), RollW(), RollH());

					// Paint top to bottom so rounding a lane to whole pixels keeps its upper grid line.
					for (int row = grid.LowRow + grid.Rows - 1; row >= grid.LowRow; --row)
					{
						const int ry = y() + (int)grid.RowY(row);
						fl_color(m_Look.Colour(NoteGrid::IsAccidental(row) ? "accidental" : "natural"));
						fl_rectf(left, ry, RollW() - (int)grid.Gutter, (int)rowHeight + 1);
						fl_color(m_Look.Colour("row_line"));
						fl_line(left, ry, x() + RollW(), ry);
						fl_color(m_Look.Colour("text"));
						fl_font(m_Look.Font("text_font"), m_Look.Size("row_text_size"));
						fl_draw(NoteGrid::NoteName(row).c_str(), x(), ry, (int)grid.Gutter - 3, (int)rowHeight, FL_ALIGN_RIGHT);
					}

					const std::vector<double> lines = grid.GridLines(256);

					for (size_t i = 0; i < lines.size(); ++i)
					{
						const int lx = x() + (int)grid.X(lines[i]);
						fl_color(m_Look.Colour("grid_line"));
						fl_line(lx, top, lx, y() + RollH());
					}

					char range[64];
					std::snprintf(range, sizeof(range), "%.2f - %.2f s", grid.First, grid.First + grid.Span);
					fl_color(m_Look.Colour("text"));
					fl_font(m_Look.Font("text_font"), m_Look.Size("header_text_size"));
					fl_draw(range, left + 2, y(), RollW() - (int)grid.Gutter - 4, (int)grid.Header, FL_ALIGN_LEFT);
					fl_pop_clip();
					fl_push_clip(left, top, RollW() - (int)grid.Gutter, RollH() - (int)grid.Header);

					for (size_t i = 0; i < grid.Notes.size(); ++i)
					{
						const GridNote &note = grid.Notes[i];

						if (grid.Dragging() && grid.Mode() != NoteGrid::Create && note.Id == grid.Original().Id)
							continue;

						DrawNote(grid, note, note.Id == grid.Selected);
					}

					if (grid.Dragging())
						DrawNote(grid, grid.Draft(), true);

					const bool marked = grid.Playhead >= grid.First && grid.Playhead <= grid.First + grid.Span;
					const int px = x() + (int)grid.X(grid.Playhead);

					if (marked)
					{
						fl_color(m_Look.Colour("playhead"));
						fl_line(px, top, px, y() + RollH());
					}

					fl_pop_clip();

					// The marker's handle, in the strip it is moved by.
					if (marked && m_Control.Marks())
					{
						fl_push_clip(left, y(), RollW() - (int)grid.Gutter, (int)grid.Header);
						fl_color(m_Look.Colour("playhead"));
						fl_polygon(px - 4, top - 8, px + 4, top - 8, px, top);
						fl_pop_clip();
					}
				}

				int handle(int event)
				{
					const double lx = Fl::event_x() - x();
					const double ly = Fl::event_y() - y();

					switch (event)
					{
						case FL_PUSH:
							// A press on a bar is the bar's.
							if (lx >= RollW() || ly >= RollH())
								return Fl_Group::handle(event);

							if (Fl::event_button() != FL_LEFT_MOUSE)
								return 1;

							take_focus();
							m_Control.Press(lx, ly);

							return 1;
						case FL_DRAG:
							m_Control.Drag(lx, ly);

							return 1;
						case FL_RELEASE:
							if (Fl::event_button() == FL_LEFT_MOUSE)
								m_Control.Release(lx, ly);

							return 1;
						case FL_KEYDOWN:
							if (Fl::event_key() == FL_Escape)
							{
								m_Control.Cancel();

								return 1;
							}

							if (Fl::event_key() == FL_Delete || Fl::event_key() == FL_BackSpace)
							{
								m_Control.RemoveSelected();

								return 1;
							}

							return 0;
						case FL_MOUSEWHEEL:
							if (Fl::event_state(FL_SHIFT))
								m_Control.Scroll(0, Fl::event_dy() / 8.0);
							else
								m_Control.Scroll(-Fl::event_dy(), 0);

							return 1;
						case FL_FOCUS:
						case FL_UNFOCUS:
						case FL_ENTER:
						case FL_LEAVE:
							return 1;
					}

					return Fl_Group::handle(event);
				}

			private:
				static void TimeMoved(Fl_Widget *bar, void *data)
				{
					PianoRollWidget *self = static_cast<PianoRollWidget *>(data);
					self->m_Control.ShowFrom(self->m_Control.Grid().LowRow, static_cast<Fl_Scrollbar *>(bar)->value() / 1000.0);
				}

				// The bar's top is the highest key.
				static void KeysMoved(Fl_Widget *bar, void *data)
				{
					PianoRollWidget *self = static_cast<PianoRollWidget *>(data);
					NoteGrid &grid = self->m_Control.Grid();
					self->m_Control.ShowFrom(Keys - grid.Rows - static_cast<Fl_Scrollbar *>(bar)->value(), grid.First);
				}

				void PlaceBars()
				{
					const NoteGrid &grid = m_Control.Grid();
					const int gutter = (int)grid.Gutter, header = (int)grid.Header;
					m_Time->resize(x() + gutter, y() + RollH(), std::max(1, RollW() - gutter), Bar);
					m_Keys->resize(x() + RollW(), y() + header, Bar, std::max(1, RollH() - header));
				}

				// The bars follow the view, in thousandths of a second and in
				// keys; one being dragged is left alone.
				void SyncBars(const NoteGrid &grid)
				{
					if (Fl::pushed() != m_Time)
					{
						m_Time->value((int)(grid.First * 1000), (int)(grid.Span * 1000), 0,
							(int)((m_Control.TimeEnd() + grid.Span) * 1000));
						m_Time->linesize(std::max(1, (int)(grid.Span * 1000 / 16)));
					}

					if (Fl::pushed() != m_Keys)
						m_Keys->value(Keys - grid.Rows - grid.LowRow, grid.Rows, 0, Keys);
				}

				void DrawNote(const NoteGrid &grid, const GridNote &note, bool marked)
				{
					if (!grid.Visible(note))
						return;

					const int nx = x() + (int)grid.X(note.Start);
					const int ny = y() + (int)grid.RowY(note.Row);
					const int width = std::max(3, (int)(grid.X(note.End) - grid.X(note.Start)));
					const int height = std::max(2, (int)grid.RowHeight() - 2);
					fl_color(fl_color_average(m_Look.Colour("note_loud"), m_Look.Colour("note_quiet"), note.Velocity));
					fl_rectf(nx, ny + 1, width, height);
					fl_color(m_Look.Colour(marked ? "note_selected" : "note_edge"));
					fl_rect(nx, ny + 1, width, height);
					if (marked && width > 2 && height > 2)
						fl_rect(nx + 1, ny + 2, width - 2, height - 2);
				}

				PianoRollControl &m_Control;
				FltkLook m_Look;
				Fl_Scrollbar *m_Time, *m_Keys;
			};

			class FltkRoll: public FltkView, public virtual RollView
			{
			public:
				explicit FltkRoll(PianoRollWidget *widget): FltkView(widget), m_Widget(widget) {}

				void Redraw() { m_Widget->redraw(); }

				void Size(double &w, double &h) const
				{
					w = m_Widget->RollW();
					h = m_Widget->RollH();
				}

			private:
				PianoRollWidget *m_Widget;
			};

			ControlView *MakePianoRoll(PanelControl &control, const Spumoni::JSON &node)
			{
				return new FltkRoll(new PianoRollWidget(control.X(), control.Y(), control.W(), control.H(),
					static_cast<PianoRollControl &>(control), FltkTheme::For(control, "PianoRoll", node)));
			}

			ViewRegistry::Entry s_PianoRoll("PianoRoll", &MakePianoRoll);
		}
	}
}
