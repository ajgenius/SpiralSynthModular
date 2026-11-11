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
			void TriggerMoved(Fl_Widget *widget, void *data)
			{
				Fl_Trigger *trigger = static_cast<Fl_Trigger *>(widget);
				static_cast<LoopWaveformControl *>(data)->TriggerMoved(trigger->GetID(), trigger->GetChannel(),
						trigger->GetAngle());
			}

			// The widget's own callbacks carry no owner; it is kept as the
			// widget's user data.
			void LoopMoved(Fl_Widget *widget, int samples)
			{
				static_cast<LoopWaveformControl *>(widget->user_data())->Moved(samples);
			}

			void LoopSought(Fl_Widget *widget, int sample)
			{
				static_cast<LoopWaveformControl *>(widget->user_data())->Sought(sample);
			}

			class FltkLoop: public FltkView, public virtual LoopView
			{
			public:
				FltkLoop(Fl_Loop *loop, LoopWaveformControl &control): FltkView(loop), m_Loop(loop), m_Control(control) {}

				void SetData(const std::vector<float> &samples) { m_Loop->SetData(&samples[0], (int)samples.size()); }
				void SetLength(int length) { m_Loop->SetLength(length); }
				int Length() const { return m_Loop->GetLength(); }

				void SetPosition(float position)
				{
					m_Loop->SetPos(position);
					m_Loop->DrawPosMarker();
				}

				void SelectAll() { m_Loop->SelectAll(); }
				void SetWaveSize(float size) { m_Loop->SetWaveSize(size); }

				void AddTrigger(int id, int channel, float angle)
				{
					Fl_Trigger *trigger = new Fl_Trigger(m_Loop->x(), m_Loop->y(), 20, 20);
					trigger->SetCentreX(m_Loop->w() / 2);
					trigger->SetCentreY(m_Loop->h() / 2);
					trigger->SetCentreRadius(m_Loop->w() * 5 / 12);
					trigger->SetAngle(angle);
					trigger->SetID(id);
					trigger->SetChannel(channel);
					trigger->callback(TriggerMoved, &m_Control);
					m_Loop->add(trigger);
					m_Loop->redraw();
				}

				long RangeStart() const { return m_Loop->GetRangeStart(); }
				long RangeEnd() const { return m_Loop->GetRangeEnd(); }
				void Redraw() { m_Loop->redraw(); }

			private:
				Fl_Loop *m_Loop;
				LoopWaveformControl &m_Control;
			};
			ControlView *MakeLoopWaveform(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_Loop *loop = new Fl_Loop(control.X(), control.Y(), control.W(), control.H());
				loop->user_data(static_cast<LoopWaveformControl *>(&control));
				loop->SetMoveCallback(LoopMoved);
				loop->SetSeekCallback(LoopSought);
				const FltkLook look = FltkTheme::For(control, "LoopWaveform", node);
				loop->SetBGColour(look.Colour("face"));
				loop->SetWaveColour(look.Colour("trace"));
				loop->SetSelColour(look.Colour("selection"));
				loop->SetIndColour(look.Colour("indicator"));
				loop->SetMrkColour(look.Colour("marker"));
				loop->end();

				return new FltkLoop(loop, static_cast<LoopWaveformControl &>(control));
			}

			ViewRegistry::Entry s_LoopWaveform("LoopWaveform", &MakeLoopWaveform);
		}
	}
}
