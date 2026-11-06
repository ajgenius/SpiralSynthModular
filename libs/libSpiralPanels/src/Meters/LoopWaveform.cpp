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
			m_TriggerCount(NULL),
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

			if (triggers)
			{
				m_TriggerCount = panel.FindProperty(TextOf(triggers->Get("count")));
				m_TriggerChannels = TextOf(triggers->Get("channel"));
				m_TriggerTimes = TextOf(triggers->Get("time"));
				m_NewTriggerCommand = TextOf(triggers->Get("new"));
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

		const PanelProperty *LoopWaveformControl::TriggerProperty(const std::string &pattern, int index) const
		{
			const std::string::size_type at = pattern.find("{index}");

			if (at == std::string::npos)
				return NULL;

			char number[16];
			std::snprintf(number, sizeof number, "%d", index);

			return m_Panel.FindProperty(pattern.substr(0, at) + number + pattern.substr(at + 7));
		}

		void LoopWaveformControl::SyncTriggers()
		{
			if (!m_TriggerCount || !m_Panel.Source())
				return;

			const int count = (int)m_Panel.Read(*m_TriggerCount);

			for (int i = (int)m_Triggers.size(); i < count; ++i)
			{
				const PanelProperty *channel = TriggerProperty(m_TriggerChannels, i);
				const PanelProperty *time = TriggerProperty(m_TriggerTimes, i);

				if (!channel || !time)
					break;

				AddTrigger(i, (int)m_Panel.Read(*channel), (float)m_Panel.Read(*time) * 360.0f);
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

			const PanelProperty *channelProperty = TriggerProperty(m_TriggerChannels, id);
			const PanelProperty *timeProperty = TriggerProperty(m_TriggerTimes, id);

			if (channelProperty)
				m_Panel.Write(*channelProperty, channel);

			if (timeProperty)
				m_Panel.Write(*timeProperty, angle / 360.0f);
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
