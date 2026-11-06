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
			class FltkMeter: public FltkView, public virtual MeterView
			{
			public:
				explicit FltkMeter(Fl_VU_Meter *meter): FltkView(meter), m_Meter(meter) {}

				void SetMode(bool vu)
				{
					if (m_Meter->vu_mode() == vu)
						return;

					m_Meter->vu_mode(vu);
					m_Meter->redraw();
				}

				void SetLevel(float level)
				{
					if (m_Meter->value() != level)
						m_Meter->value(level);
				}

			private:
				Fl_VU_Meter *m_Meter;
			};
			ControlView *MakeMeterBar(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_VU_Meter *meter = new Fl_VU_Meter(control.X(), control.Y(), control.W(), control.H());
				FltkTheme::For(control, "MeterBar", node).Apply(meter);
				meter->vu_mode(true);

				return new FltkMeter(meter);
			}

			ViewRegistry::Entry s_MeterBar("MeterBar", &MakeMeterBar);
		}
	}
}
