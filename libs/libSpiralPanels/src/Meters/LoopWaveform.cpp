// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
#include "ControlSupport.h"
#include <cstdio>

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_LoopWaveform("LoopWaveform", &LoopWaveformControl::Create);

		PanelControl *LoopWaveformControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new LoopWaveformControl(panel, node));
		}

		LoopWaveformControl::LoopWaveformControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Loop(MakeView<LoopView>("LoopWaveform", *this, node)),
			m_Pos(panel.FindProperty(TextOf(node.Get("pos")))),
			m_Length(panel.FindProperty(TextOf(node.Get("length")))),
			m_TriggerRows(NULL),
			m_ChannelField(-1),
			m_TimeField(-1),
			m_Fetched(false)
		{
			const Spumoni::JSON *sample = node.Get("sample");

			if (sample)
			{
				m_SampleCommand = TextOf(sample->Get("command"));
				m_SizeChannel = TextOf(sample->Get("size"));
				m_BufferChannel = TextOf(sample->Get("buffer"));
			}

			const Spumoni::JSON *triggers = node.Get("triggers");

			// The trigger points are rows of one of the panel's sources: which
			// of its fields is a point's output and which its place.
			if (triggers)
			{
				m_TriggerRows = panel.Rows(TextOf(triggers->Get("source")));
				m_NewTriggerCommand = TextOf(triggers->Get("new"));

				if (m_TriggerRows)
				{
					m_ChannelField = m_TriggerRows->FindField(TextOf(triggers->Get("channel")));
					m_TimeField = m_TriggerRows->FindField(TextOf(triggers->Get("time")));
				}
			}

			if (!m_Loop)
				return;

			m_Loop->SetLength(0);
			Adopt(m_Loop);
		}

		void LoopWaveformControl::Load()
		{
			ShowLength();
			SyncTriggers();
		}

		void LoopWaveformControl::ShowLength()
		{
			m_Loop->SetLength(m_Length && !m_Samples.empty() ? (int)m_Panel.Read(*m_Length) : 0);
		}

		void LoopWaveformControl::Refresh()
		{
			if (!m_Fetched)
				Reload();

			if (m_Pos)
				m_Loop->SetPosition((float)m_Panel.Read(*m_Pos));
		}

		void LoopWaveformControl::Reload()
		{
			m_Fetched = true;

			if (m_SampleCommand.empty() || !m_Panel.Source())
				return;

			if (!m_Panel.Source()->FetchFloats(m_SampleCommand, m_SizeChannel, m_BufferChannel, m_Samples))
				return;

			if (!m_Samples.empty())
				m_Loop->SetData(m_Samples);

			m_Loop->Redraw();
		}

		void LoopWaveformControl::AddTrigger(int id, int channel, float angle)
		{
			Trigger trigger = {id, channel, angle};
			m_Triggers.push_back(trigger);
			m_Loop->AddTrigger(id, channel, angle);
		}

		void LoopWaveformControl::SyncTriggers()
		{
			if (!m_TriggerRows || !m_Panel.Source())
				return;

			const long count = m_TriggerRows->Count();

			for (long i = (long)m_Triggers.size(); i < count; ++i)
			{
				Spiral::Data::ControlValue channel, time;

				if (!m_TriggerRows->GetAt(i, m_ChannelField, channel) || !m_TriggerRows->GetAt(i, m_TimeField, time))
					break;

				AddTrigger((int)i, (int)channel.number, (float)time.number * 360.0f);
			}
		}

		void LoopWaveformControl::NewTrigger()
		{
			const int id = (int)m_Triggers.size();
			AddTrigger(id, id < 8 ? id : 7, 0);
			const Trigger &trigger = m_Triggers.back();
			std::vector<std::string> args;
			args.push_back(TextOfNumber(trigger.Id));
			args.push_back(TextOfNumber(trigger.Channel));
			args.push_back(TextOfNumber(trigger.Angle / 360.0f));
			m_Panel.Command(m_NewTriggerCommand, args);
		}

		void LoopWaveformControl::TriggerMoved(int id, int channel, float angle)
		{
			for (size_t i = 0; i < m_Triggers.size(); ++i)
			{
				if (m_Triggers[i].Id == id)
				{
					m_Triggers[i].Channel = channel;
					m_Triggers[i].Angle = angle;
				}
			}

			if (!m_TriggerRows || m_Panel.Loading())
				return;

			m_TriggerRows->SetAt(id, m_ChannelField, Spiral::Data::ControlValue((double)channel));
			m_TriggerRows->SetAt(id, m_TimeField, Spiral::Data::ControlValue((double)(angle / 360.0f)));
		}

		// Turning the loop and seeking change nothing here: the panel's
		// handlers send them on to the device.
		static const PanelControl::Event MovedEvent = {"moved", {"value", "text", NULL}};
		static const PanelControl::Event SoughtEvent = {"sought", {"value", "text", NULL}};

		void LoopWaveformControl::DeclareEvents(std::vector<Event> &out) const
		{
			PanelControl::DeclareEvents(out);
			out.push_back(MovedEvent);
			out.push_back(SoughtEvent);
		}

		void LoopWaveformControl::Moved(int samples)
		{
			Fire("moved", samples);
		}

		void LoopWaveformControl::Sought(int sample)
		{
			Fire("sought", sample);
		}

		void LoopWaveformControl::SetAttribute(const std::string &attribute, double value)
		{
			if (attribute == "reload")
				Reload();
			else if (attribute == "select_all")
				m_Loop->SelectAll();
			else if (attribute == "new_trigger")
				NewTrigger();
			else if (attribute == "wave_size")
			{
				m_Loop->SetWaveSize((float)value);
				m_Loop->Redraw();
			}
			else if (attribute == "length")
			{
				m_Loop->SetLength((int)(m_Samples.size() * value));
				m_Loop->Redraw();
			}
		}

		bool LoopWaveformControl::GetAttribute(const std::string &attribute, double &value) const
		{
			if (attribute == "range_start")
				value = (double)m_Loop->RangeStart();
			else if (attribute == "range_end")
				value = (double)m_Loop->RangeEnd();
			else if (attribute == "wave_size")
				value = 1;
			else if (attribute == "length")
				value = m_Samples.empty() ? 1 : (double)m_Loop->Length() / (double)m_Samples.size();
			else
				return false;

			return true;
		}
	}
}
