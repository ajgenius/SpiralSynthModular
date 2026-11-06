// SPDX-License-Identifier: GPL-2.0-or-later
#include "FltkSupport.h"
#include "FltkTheme.h"
#include "Fl_LED_Button.H"

namespace Spiral
{
	namespace Panels
	{
		namespace
		{
			ControlView *MakeLEDButton(PanelControl &control, const Spumoni::JSON &node)
			{
				ButtonControl &owner = static_cast<ButtonControl &>(control);
				Fl_LED_Button *button = new Fl_LED_Button(control.X(), control.Y(), control.W(), control.H());

				if (node.Get("down") && node.Get("down")->AsBool())
					button->value(1);

				button->copy_label(owner.Label().c_str());
				FltkTheme::For(control, "LEDButton", node, "", FltkTheme::SizeLabel).Apply(button);
				button->type(owner.ButtonMode() == ButtonControl::Radio ? FL_RADIO_BUTTON : FL_TOGGLE_BUTTON);
				button->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
				button->callback(ButtonChanged, &owner);

				return new FltkButton(button);
			}

			ViewRegistry::Entry s_LEDButton("LEDButton", &MakeLEDButton);
		}
	}
}
