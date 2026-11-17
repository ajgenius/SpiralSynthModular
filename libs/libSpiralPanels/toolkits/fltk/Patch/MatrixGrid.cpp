// SPDX-License-Identifier: GPL-2.0-or-later
#include "Patch.h"
#include "FltkTheme.h"
#include "FltkView.h"
#include "JSON.h"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <algorithm>
#include <climits>

namespace Spiral
{
	namespace Panels
	{
		namespace
		{
			static const char *NoteText[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

			// The volume slider sits in the grid's parent, which may delete
			// it before the grid or after; it tells the grid when it goes.
			class VolumeSlider: public Fl_Slider
			{
			public:
				VolumeSlider(int x, int y, int w, int h, Fl_Slider **owner): Fl_Slider(x, y, w, h), m_Owner(owner) {}

				~VolumeSlider()
				{
					if (*m_Owner == this)
						*m_Owner = NULL;
				}

			private:
				Fl_Slider **m_Owner;
			};

			class MatrixWidget: public Fl_Widget
			{
			public:
				MatrixWidget(int x, int y, int w, int h, MatrixGridControl &control, bool classic, const FltkLook &look):
					Fl_Widget(x, y, w, h),
					m_Control(control),
					m_Classic(classic),
					m_Look(look),
					m_HoverC(-1),
					m_HoverR(0),
					m_Slider(NULL),
					m_SliderC(-1),
					m_SliderR(0)
				{
				}

				~MatrixWidget()
				{
					delete m_Slider;
				}

				void draw()
				{
					NoteGrid &grid = m_Control.Grid();
					const int columns = m_Control.Columns();
					const int rows = m_Control.Rows();
					const int gutter = (int)grid.Gutter;
					const int x0 = x() + gutter;
					const int w0 = w() - gutter;
					fl_color(m_Look.Colour("face"));
					fl_rectf(x(), y(), w(), h());

					for (int c = 0; c < columns; ++c)
					{
						for (int r = 0; r < rows; ++r)
						{
							const int cx = x0 + c * w0 / columns;
							const int cw = x0 + (c + 1) * w0 / columns - cx;
							const int ry = y() + (rows - 1 - r) * h() / rows;
							const int rh = y() + (rows - r) * h() / rows - ry;
							const int start = m_Control.NoteAt(c, r);
							const bool joined = start >= 0 && start < c;
							const Fl_Color colour = CellColour(c, r, start);
							fl_color(colour);

							if (m_Classic)
							{
								fl_rectf(cx, ry, cw, rh);
								fl_color(m_Look.Colour("cell_edge"));
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
					}

					const bool dragging = grid.Dragging();
					const int hc = dragging ? (int)grid.Draft().Start : m_HoverC;
					const int hr = dragging ? grid.Draft().Row : m_HoverR;

					if (hc >= 0 && m_Control.NoteAt(hc, hr) == hc)
					{
						const int left = x0 + hc * w0 / columns;
						const int right = x0 + m_Control.EndOf(hc, hr) * w0 / columns;
						const int ry = y() + (rows - 1 - hr) * h() / rows;
						const int rh = y() + (rows - hr) * h() / rows - ry;
						fl_color(m_Look.Colour("hover"));
						fl_rect(left, ry, right - left + 1, rh + 1);
						fl_rectf(right - 2, ry + 1, 2, rh - 1);
					}

					if (gutter <= 0)
						return;

					fl_font(m_Look.Font("text_font"), m_Look.Size("text_size"));
					const int trigger = m_Control.TriggerRow();

					for (int r = 0; r < rows; ++r)
					{
						const int ry = y() + (rows - 1 - r) * h() / rows;
						const int rh = y() + (rows - r) * h() / rows - ry;

						if (trigger != INT_MIN && r >= trigger && r < trigger + MatrixGridControl::Triggers)
						{
							fl_color(fl_color_average(m_Look.Colour("trigger"), m_Look.Colour("gutter"), r == trigger ? 0.7f : 0.3f));
							fl_rectf(x(), ry, gutter, rh);
						}

						fl_color(m_Look.Colour("text"));
						fl_draw(NoteText[r % 12], x() + 2, ry, gutter - 2, rh, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
					}
				}

				int handle(int event)
				{
					NoteGrid &grid = m_Control.Grid();
					const double lx = Fl::event_x() - x();
					const double ly = Fl::event_y() - y();
					const bool names = grid.Gutter > 0 && lx < grid.Gutter;

					switch (event)
					{
						case FL_PUSH:
							// The original's volume slider: a right click on a lit
							// step opens it under the step, another puts it away,
							// and so does a left click anywhere on the grid.
							if (Fl::event_button() == FL_RIGHT_MOUSE && !names)
							{
								const int column = Column(lx, grid);
								const int row = grid.RowAt(ly);
								const int start = column >= 0 ? m_Control.NoteAt(column, row) : -1;
								const bool same = m_Slider && start == m_SliderC && row == m_SliderR;
								CloseSlider();

								if (start >= 0 && !same && m_Control.HasVolume())
									OpenSlider(start, row);

								return 1;
							}

							if (Fl::event_button() != FL_LEFT_MOUSE)
								return 1;

							CloseSlider();

							if (names && m_Control.HasTriggers())
							{
								m_Control.StartMarking(grid.RowAt(ly));

								return 1;
							}

							m_Control.Press(lx, ly);

							return 1;
						case FL_DRAG:
							if (m_Control.Marking())
								m_Control.MarkTo(grid.RowAt(std::max(0.0, std::min(ly, (double)h() - 1))));
							else
								m_Control.Drag(lx, ly);

							return 1;
						case FL_RELEASE:
							if (m_Control.Marking())
								m_Control.EndMarking();
							else if (Fl::event_button() == FL_LEFT_MOUSE)
								m_Control.Release(lx, ly);

							return 1;
						case FL_ENTER:
						case FL_MOVE:
							Hover(names ? -1 : Column(lx, grid), grid.RowAt(ly));

							return 1;
						case FL_LEAVE:
							Hover(-1, 0);

							return 1;
						case FL_MOUSEWHEEL:
							m_Control.Wheel(lx, ly, Fl::event_dy());

							return 1;
					}

					return Fl_Widget::handle(event);
				}

			private:
				static const int SliderHeight = 50;

				static void SliderMoved(Fl_Widget *slider, void *data)
				{
					MatrixWidget *self = static_cast<MatrixWidget *>(data);
					const float volume = 1.0f - (float)(static_cast<Fl_Slider *>(slider)->value() / 255.0);
					self->m_Control.SetVolumeAt(self->m_SliderC, self->m_SliderR, volume);
				}

				void OpenSlider(int column, int row)
				{
					NoteGrid &grid = m_Control.Grid();
					const int columns = m_Control.Columns();
					const int rows = m_Control.Rows();
					const int x0 = x() + (int)grid.Gutter;
					const int w0 = w() - (int)grid.Gutter;
					const int cx = x0 + column * w0 / columns;
					const int cw = std::max(12, x0 + (column + 1) * w0 / columns - cx);
					const int top = y() + (rows - 1 - row) * h() / rows;
					const int below = y() + (rows - row) * h() / rows;
					Fl_Group *group = parent();

					// Under the step, or over it where the panel ends first.
					const bool fits = !group || below + SliderHeight <= group->y() + group->h();
					Fl_Group::current(NULL);
					m_Slider = new VolumeSlider(cx, fits ? below : top - SliderHeight, cw, SliderHeight, &m_Slider);
					m_Slider->type(FL_VERT_NICE_SLIDER);
					m_Slider->selection_color(m_Look.Colour("note_loud"));
					m_Slider->maximum(255);
					m_Slider->step(1);
					m_Slider->value(255 - 255 * std::max(0.0f, std::min(1.0f, m_Control.VolumeAt(column, row))));
					m_Slider->callback(&SliderMoved, this);
					m_SliderC = column;
					m_SliderR = row;

					if (group)
					{
						group->add(m_Slider);
						group->redraw();
					}
				}

				void CloseSlider()
				{
					if (!m_Slider)
						return;

					Fl_Group *group = m_Slider->parent();
					m_Slider->hide();

					if (group)
					{
						group->remove(m_Slider);
						group->redraw();
					}

					// Deleted once the event is over, by when another may be open.
					Fl_Slider *gone = m_Slider;
					m_Slider = NULL;
					m_SliderC = -1;
					Fl::delete_widget(gone);
				}

				int Column(double lx, const NoteGrid &grid) const
				{
					const int column = (int)grid.TimeAt(lx);

					return column >= 0 && column < m_Control.Columns() ? column : -1;
				}

				void Hover(int column, int row)
				{
					const int start = column >= 0 && row >= 0 ? m_Control.NoteAt(column, row) : -1;

					if (start == m_HoverC && (start < 0 || row == m_HoverR))
						return;

					m_HoverC = start;
					m_HoverR = row;
					redraw();
				}

				Fl_Color CellColour(int c, int r, int start) const
				{
					if (start >= 0)
						c = start;

					const float volume = std::max(0.0f, std::min(1.0f, m_Control.VolumeAt(c, r)));

					if (start >= 0)
						return m_Control.HasVolumes() ? fl_color_average(m_Look.Colour("note_loud"), m_Look.Colour("note_quiet"), volume)
								: m_Look.Colour("note");

					if (m_Classic)
					{
						if (c % 8 == 0)
							return m_Look.Colour("bar");

						return m_Look.Colour(NoteGrid::IsAccidental(r) ? "accidental" : "natural");
					}

					if (c == m_Control.Step())
						return m_Look.Colour("step");

					return m_Look.Colour(c % 4 ? "cell" : "beat");
				}

				MatrixGridControl &m_Control;
				bool m_Classic;
				FltkLook m_Look;
				int m_HoverC, m_HoverR;
				Fl_Slider *m_Slider;
				int m_SliderC, m_SliderR;
			};

			class FltkMatrix: public FltkView, public virtual RollView
			{
			public:
				explicit FltkMatrix(MatrixWidget *widget): FltkView(widget), m_Widget(widget) {}

				void Redraw() { m_Widget->redraw(); }

				void Size(double &w, double &h) const
				{
					w = m_Widget->w();
					h = m_Widget->h();
				}

			private:
				MatrixWidget *m_Widget;
			};

			ControlView *MakeMatrixGrid(PanelControl &control, const Spumoni::JSON &node)
			{
				const std::string scheme = FltkTheme::For(control, "MatrixGrid", node).Text("scheme");

				return new FltkMatrix(new MatrixWidget(control.X(), control.Y(), control.W(), control.H(),
					static_cast<MatrixGridControl &>(control), scheme != "fade",
					FltkTheme::For(control, "MatrixGrid", node, scheme)));
			}

			ViewRegistry::Entry s_MatrixGrid("MatrixGrid", &MakeMatrixGrid);
		}
	}
}
