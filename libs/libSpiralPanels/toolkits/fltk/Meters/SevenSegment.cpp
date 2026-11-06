// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
#include "FltkTheme.h"
#include "FltkSupport.h"
#include "Fl_LED_Button.H"
#include "Fl_Loop.h"
#include "Fl_SevenSeg.H"
#include "Fl_Trigger.h"
#include "Fl_VU_Meter.h"
#include "Fl_WaveDisplay.H"
#include "JSON.h"
#include "ScopeView.h"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <cstring>

namespace Spiral
{
	namespace Panels
	{
		namespace
		{
			class FltkDigits: public FltkView, public virtual DigitsView
			{
			public:
				FltkDigits(Fl_Group *group, const std::vector<Fl_SevenSeg *> &digits): FltkView(group), m_Digits(digits) {}

				void SetDigit(size_t index, int value, Dot dot)
				{
					if (index >= m_Digits.size())
						return;

					m_Digits[index]->dp(dot == Colon ? colon : dot == Point ? point : off);
					m_Digits[index]->value(value);
				}

			private:
				std::vector<Fl_SevenSeg *> m_Digits;
			};
			ControlView *MakeSevenSegment(PanelControl &control, const Spumoni::JSON &node)
			{
				const SevenSegmentControl &display = static_cast<SevenSegmentControl &>(control);
				const int digits = display.Digits();
				const int width = digits > 0 ? control.W() / digits : control.W();
				Fl_Group *group = new Fl_Group(control.X(), control.Y(), control.W(), control.H());
				std::vector<Fl_SevenSeg *> made;
				const FltkLook look = FltkTheme::For(control, "SevenSegment", node);

				for (int dis = 0; dis < digits; ++dis)
				{
					Fl_SevenSeg *digit = new Fl_SevenSeg(control.X() + width * dis, control.Y(), width, control.H());
					digit->bar_width((int)Number(node, "bar_width", 4));
					look.Apply(digit);
					digit->color2(look.Colour("color2"));

					if (display.Time() && dis > 0 && dis % 2 == 0)
						digit->dp(colon);

					made.push_back(digit);
				}

				group->end();

				return new FltkDigits(group, made);
			}

			ViewRegistry::Entry s_SevenSegment("SevenSegment", &MakeSevenSegment);
		}
	}
}
