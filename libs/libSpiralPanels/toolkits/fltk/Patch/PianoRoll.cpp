// SPDX-License-Identifier: GPL-2.0-or-later
#include "Patch.h"
#include "FltkView.h"
#include "FltkTheme.h"
#include "JSON.h"
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <algorithm>
#include <cstdio>

namespace Spiral
{
	namespace Panels
	{
		namespace
		{
			class PianoRollWidget: public Fl_Widget
			{
			public:
				PianoRollWidget(int x, int y, int w, int h, PianoRollControl &control, const FltkLook &look):
					Fl_Widget(x, y, w, h),
					m_Control(control),
					m_Look(look)
				{
					look.Apply(this);
				}

				void draw()
				{
					NoteGrid &grid = m_Control.Grid();
					const int left = x() + (int)grid.Gutter;
					const int top = y() + (int)grid.Header;
					const double rowHeight = grid.RowHeight();
					fl_color(m_Look.Colour("face"));
					fl_rectf(x(), y(), w(), h());
					fl_push_clip(x(), y(), w(), h());

					for (int row = grid.LowRow; row < grid.LowRow + grid.Rows; ++row)
					{
						const int ry = y() + (int)grid.RowY(row);
						fl_color(m_Look.Colour(NoteGrid::IsAccidental(row) ? "accidental" : "natural"));
						fl_rectf(left, ry, w() - (int)grid.Gutter, (int)rowHeight + 1);
						fl_color(m_Look.Colour("row_line"));
						fl_line(left, ry, x() + w(), ry);
						fl_color(m_Look.Colour("text"));
						fl_font(m_Look.Font("text_font"), m_Look.Size("row_text_size"));
						fl_draw(NoteGrid::NoteName(row).c_str(), x(), ry, (int)grid.Gutter - 3, (int)rowHeight, FL_ALIGN_RIGHT);
					}

					const std::vector<double> lines = grid.GridLines(256);

					for (size_t i = 0; i < lines.size(); ++i)
					{
						const int lx = x() + (int)grid.X(lines[i]);
						fl_color(m_Look.Colour("grid_line"));
						fl_line(lx, top, lx, y() + h());
					}

					char range[64];
					std::snprintf(range, sizeof(range), "%.2f - %.2f s", grid.First, grid.First + grid.Span);
					fl_color(m_Look.Colour("text"));
					fl_font(m_Look.Font("text_font"), m_Look.Size("header_text_size"));
					fl_draw(range, left + 2, y(), w() - (int)grid.Gutter - 4, (int)grid.Header, FL_ALIGN_LEFT);
					fl_pop_clip();
					fl_push_clip(left, top, w() - (int)grid.Gutter, h() - (int)grid.Header);

					for (size_t i = 0; i < grid.Notes.size(); ++i)
					{
						const GridNote &note = grid.Notes[i];

						if (grid.Dragging() && grid.Mode() != NoteGrid::Create && note.Id == grid.Original().Id)
							continue;

						DrawNote(grid, note, note.Id == grid.Selected);
					}

					if (grid.Dragging())
						DrawNote(grid, grid.Draft(), true);

					if (grid.Playhead >= grid.First && grid.Playhead <= grid.First + grid.Span)
					{
						const int px = x() + (int)grid.X(grid.Playhead);
						fl_color(m_Look.Colour("playhead"));
						fl_line(px, top, px, y() + h());
					}

					fl_pop_clip();
				}

				int handle(int event)
				{
					const double lx = Fl::event_x() - x();
					const double ly = Fl::event_y() - y();

					switch (event)
					{
						case FL_PUSH:
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

					return Fl_Widget::handle(event);
				}

			private:
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
				}

				PianoRollControl &m_Control;
				FltkLook m_Look;
			};

			class FltkRoll: public FltkView, public virtual RollView
			{
			public:
				explicit FltkRoll(PianoRollWidget *widget): FltkView(widget), m_Widget(widget) {}

				void Redraw() { m_Widget->redraw(); }

				void Size(double &w, double &h) const
				{
					w = m_Widget->w();
					h = m_Widget->h();
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
