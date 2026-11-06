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
			class FltkLED: public FltkView, public virtual CheckView
			{
			public:
				explicit FltkLED(Fl_LED_Button *button): FltkView(button), m_Button(button) {}

				void SetChecked(bool checked) { m_Button->value(checked); }

			private:
				Fl_LED_Button *m_Button;
			};
			void LEDChanged(Fl_Widget *widget, void *data)
			{
				static_cast<LEDControl *>(data)->Changed(static_cast<Fl_LED_Button *>(widget)->value() != 0);
			}

			ControlView *MakeLED(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_LED_Button *button = new Fl_LED_Button(control.X(), control.Y(), control.W(), control.H());
				button->copy_label(Text(node, "text").c_str());
				button->type(FL_TOGGLE_BUTTON);
				FltkTheme::For(control, "LED", node).Apply(button);

				button->clear_visible_focus();
				button->callback(LEDChanged, static_cast<LEDControl *>(&control));

				return new FltkLED(button);
			}

			ViewRegistry::Entry s_LED("LED", &MakeLED);
		}
	}
}
