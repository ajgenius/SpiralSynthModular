// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
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
			class FltkTrace: public FltkView, public virtual TraceView
			{
			public:
				explicit FltkTrace(ScopeView *view): FltkView(view), m_View(view) {}

				void SetSamples(const std::vector<float> &samples) { m_View->SetSamples(samples); }
				void SetAttenuation(double attenuation) { m_View->SetAttenuation(attenuation); }
				void SetTimeBase(double timeBase) { m_View->SetTimeBase(timeBase); }

			private:
				ScopeView *m_View;
			};
			ControlView *MakeScope(PanelControl &control, const Spumoni::JSON &node)
			{
				return new FltkTrace(new ScopeView(control.X(), control.Y(), control.W(), control.H(),
					FltkTheme::For(control, "Scope", node)));
			}

			ViewRegistry::Entry s_Scope("Scope", &MakeScope);
		}
	}
}
