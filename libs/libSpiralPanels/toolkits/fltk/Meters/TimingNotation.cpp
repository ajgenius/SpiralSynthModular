// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
#include "FltkSupport.h"
#include "TimingNotationDrawing.h"
#include <FL/fl_draw.H>

namespace Spiral
{
	namespace Panels
	{
		namespace
		{
			class Staff: public Fl_Widget
			{
			public:
				Staff(int x, int y, int w, int h): Fl_Widget(x, y, w, h), bpm(120), note(0), beats(4), unit(4) {}

				void draw()
				{
					fl_push_clip(x(), y(), w(), h());
					fl_color(FL_BACKGROUND_COLOR);
					fl_rectf(x(), y(), w(), h());
					fl_color(FL_FOREGROUND_COLOR);
					DrawTimingNotation(*this, bpm, note, beats, unit);
					fl_pop_clip();
				}

				void Line(int a, int b, int c, int d) { fl_line(x()+a, y()+b, x()+c, y()+d); }
				void Head(int a, int b, bool hollow)
				{
					if (hollow)
						fl_arc(x()+a, y()+b, 10, 7, 0, 360);
					else
						fl_pie(x()+a, y()+b, 10, 7, 0, 360);
				}
				void Dot(int a, int b) { fl_pie(x()+a, y()+b, 3, 3, 0, 360); }
				void Flag(int a, int b)
				{
					fl_begin_line();
					fl_curve(x()+a, y()+b, x()+a, y()+b+4, x()+a+9, y()+b+5, x()+a+7, y()+b+12);
					fl_end_line();
				}
				void Words(const char *text, int a, int b, int size)
				{
					fl_font(FL_HELVETICA, size);
					fl_draw(text, x()+a, y()+b+fl_height()-fl_descent());
				}

				double bpm;
				int note, beats, unit;
			};

			class StaffView: public FltkView, public virtual TimingNotationView
			{
			public:
				explicit StaffView(Staff *staff): FltkView(staff), m_Staff(staff) {}

				void SetTiming(double bpm, int note, int beats, int unit)
				{
					if (m_Staff->bpm == bpm && m_Staff->note == note && m_Staff->beats == beats && m_Staff->unit == unit)
						return;

					m_Staff->bpm = bpm;
					m_Staff->note = note;
					m_Staff->beats = beats;
					m_Staff->unit = unit;
					m_Staff->redraw();
				}

			private:
				Staff *m_Staff;
			};

			ControlView *MakeTimingNotation(PanelControl &control, const Spumoni::JSON &)
			{
				return new StaffView(new Staff(control.X(), control.Y(), control.W(), control.H()));
			}

			ViewRegistry::Entry registration("TimingNotation", &MakeTimingNotation);
		}
	}
}
