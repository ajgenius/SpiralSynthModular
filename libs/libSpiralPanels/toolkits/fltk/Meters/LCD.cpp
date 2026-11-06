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
			class FltkReadout: public FltkView, public virtual ReadoutView
			{
			public:
				explicit FltkReadout(Fl_Output *output): FltkView(output), m_Output(output) {}

				void SetReadout(const std::string &text, int cursor)
				{
					if (cursor < 0 && !std::strcmp(text.c_str(), m_Output->value()))
						return;

					m_Output->value(text.c_str());

					if (cursor >= 0)
						m_Output->position(cursor);
				}

			private:
				Fl_Output *m_Output;
			};
			ControlView *MakeLCD(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_Output *output = new Fl_Output(control.X(), control.Y(), control.W(), control.H());
				const FltkLook look = FltkTheme::For(control, "LCD", node, "", FltkTheme::SizeText);
				look.Apply(output);
				look.ApplyText(output);

				return new FltkReadout(output);
			}

			ViewRegistry::Entry s_LCD("LCD", &MakeLCD);
		}
	}
}
