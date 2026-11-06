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
			class FltkSample: public FltkView, public virtual SampleView
			{
			public:
				explicit FltkSample(Fl_WaveDisplay *display): FltkView(display), m_Display(display) {}

				void SetSample(const std::vector<float> &samples)
				{
					if (samples.empty())
						m_Display->ClearSample();
					else
						m_Display->SetSample(&samples[0], (long)samples.size());
				}

				void SetMarker(Marker marker, long position)
				{
					if (marker == PlayStart)
						m_Display->SetPlayStart(position);
					else if (marker == LoopStart)
						m_Display->SetLoopStart(position);
					else
						m_Display->SetLoopEnd(position);
				}

				long GetMarker(Marker marker) const
				{
					if (marker == PlayStart)
						return m_Display->GetPlayStart();

					return marker == LoopStart ? m_Display->GetLoopStart() : m_Display->GetLoopEnd();
				}

				void SetPlayPosition(long position) { m_Display->SetPlayPos(position); }
				void ShowPositionMarker(bool shown) { m_Display->SetPosMarker(shown); }

				void Zoom(bool in)
				{
					if (in)
						m_Display->ZoomIn();
					else
						m_Display->ZoomOut();
				}

				long RangeStart() const { return m_Display->GetRangeStart(); }
				long RangeEnd() const { return m_Display->GetRangeEnd(); }
				bool Displayed() const { return m_Display->visible(); }
				void Redraw() { m_Display->redraw(); }

			private:
				Fl_WaveDisplay *m_Display;
			};
			void MarkerMoved(Fl_Widget *, void *data)
			{
				static_cast<SampleWaveformControl *>(data)->MarkerMoved();
			}

			ControlView *MakeSampleWaveform(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_WaveDisplay *display = new Fl_WaveDisplay(control.X(), control.Y(), control.W(), control.H(), "");
				const FltkLook look = FltkTheme::For(control, "SampleWaveform", node);
				display->SetColours(look.Colour("face"), look.Colour("trace"), look.Colour("selection"),
					look.Colour("indicator"), look.Colour("marker"));
				display->callback(MarkerMoved, static_cast<SampleWaveformControl *>(&control));

				return new FltkSample(display);
			}

			ViewRegistry::Entry s_SampleWaveform("SampleWaveform", &MakeSampleWaveform);
		}
	}
}
