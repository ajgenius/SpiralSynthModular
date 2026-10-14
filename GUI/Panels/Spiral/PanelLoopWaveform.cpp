// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "PanelLoopWaveform.h"
#include "Panel.h"
#include "DeviceSource.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include "Fl_Loop.h"
#include "Fl_Trigger.h"
#include <FL/Fl.H>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("LoopWaveform", &PanelLoopWaveform::Create);

	PanelControl *PanelLoopWaveform::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelLoopWaveform(panel, node);
	}

	PanelLoopWaveform::PanelLoopWaveform(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
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
		m_Loop = new Fl_Loop(m_X, m_Y, m_W, m_H);
		m_Loop->SetBGColour(fl_rgb_color(20, 60, 20));
		m_Loop->SetWaveColour(fl_rgb_color(100, 200, 100));
		m_Loop->SetSelColour(FL_WHITE);
		m_Loop->SetIndColour(fl_rgb_color(203, 255, 0));
		m_Loop->SetMrkColour(fl_rgb_color(155, 155, 50));
		m_Loop->end();
		m_Loop->SetLength(0);
		Adopt(m_Loop);
	}

	void PanelLoopWaveform::Load()
	{
		ShowLength();
		SyncTriggers();
	}

	void PanelLoopWaveform::ShowLength()
	{
		if (m_Length && !m_Samples.empty())
			m_Loop->SetLength((int)m_Panel.Read(*m_Length));
		else
			m_Loop->SetLength(0);
	}

	void PanelLoopWaveform::Refresh()
	{
		if (!m_Fetched)
			Reload();
		if (m_Pos)
		{
			m_Loop->SetPos((float)m_Panel.Read(*m_Pos));
			m_Loop->DrawPosMarker();
		}
	}

	void PanelLoopWaveform::Reload()
	{
		m_Fetched = true;
		if (m_SampleCommand.empty() || !m_Panel.Source())
			return;
		if (!m_Panel.Source()->FetchFloats(m_SampleCommand, m_SizeChannel, m_BufferChannel, m_Samples))
			return;
		if (!m_Samples.empty())
			m_Loop->SetData(&m_Samples[0], (int)m_Samples.size());
		m_Loop->redraw();
	}

	Fl_Trigger *PanelLoopWaveform::AddTrigger(int id, int channel, float angle)
	{
		Fl_Trigger *trigger = new Fl_Trigger(m_Loop->x(), m_Loop->y(), 20, 20);
		trigger->SetCentreX(m_Loop->w() / 2);
		trigger->SetCentreY(m_Loop->h() / 2);
		trigger->SetCentreRadius(m_Loop->w() * 5 / 12);
		trigger->SetAngle(angle);
		trigger->SetID(id);
		trigger->SetChannel(channel);
		trigger->callback(TriggerMoved, this);
		m_Loop->add(trigger);
		m_Triggers.push_back(trigger);
		m_Loop->redraw();
		return trigger;
	}

	const PanelProperty *PanelLoopWaveform::TriggerProperty(const std::string &pattern, int index) const
	{
		const std::string::size_type at = pattern.find("{index}");
		if (at == std::string::npos)
			return NULL;
		char number[16];
		snprintf(number, sizeof number, "%d", index);
		return m_Panel.FindProperty(pattern.substr(0, at) + number + pattern.substr(at + 7));
	}

	void PanelLoopWaveform::SyncTriggers()
	{
		if (!m_TriggerCount || !m_Panel.Source())
			return;
		const int count = (int)m_Panel.Read(*m_TriggerCount);
		for (int i = (int)m_Triggers.size(); i < count; ++i)
		{
			const PanelProperty *channel = TriggerProperty(m_TriggerChannels, i), *time = TriggerProperty(m_TriggerTimes, i);
			if (!channel || !time)
				break;
			AddTrigger(i, (int)m_Panel.Read(*channel), (float)m_Panel.Read(*time) * 360.0f);
		}
	}

	void PanelLoopWaveform::NewTrigger()
	{
		const int id = (int)m_Triggers.size();
		Fl_Trigger *trigger = AddTrigger(id, id < 8 ? id : 7, 0);
		m_Panel.Command(m_NewTriggerCommand, TriggerArgs(trigger));
	}

	std::vector<std::string> PanelLoopWaveform::TriggerArgs(Fl_Trigger *trigger)
	{
		std::vector<std::string> args;
		args.push_back(TextOfNumber(trigger->GetID()));
		args.push_back(TextOfNumber(trigger->GetChannel()));
		args.push_back(TextOfNumber(trigger->GetAngle() / 360.0f));
		return args;
	}

	void PanelLoopWaveform::TriggerMoved(Fl_Widget *widget, void *data)
	{
		PanelLoopWaveform *self = (PanelLoopWaveform *)data;
		Fl_Trigger *trigger = (Fl_Trigger *)widget;
		const PanelProperty *channel = self->TriggerProperty(self->m_TriggerChannels, trigger->GetID());
		const PanelProperty *time = self->TriggerProperty(self->m_TriggerTimes, trigger->GetID());
		if (channel) self->m_Panel.Write(*channel, trigger->GetChannel());
		if (time) self->m_Panel.Write(*time, trigger->GetAngle() / 360.0f);
	}

	void PanelLoopWaveform::SetAttribute(const std::string &attribute, double value)
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
			m_Loop->redraw();
		}
		else if (attribute == "length")
		{
			m_Loop->SetLength((int)(m_Samples.size() * value));
			m_Loop->redraw();
		}
	}

	bool PanelLoopWaveform::GetAttribute(const std::string &attribute, double &value) const
	{
		if (attribute == "range_start") value = (double)m_Loop->GetRangeStart();
		else if (attribute == "range_end") value = (double)m_Loop->GetRangeEnd();
		else if (attribute == "wave_size") value = 1;
		else if (attribute == "length")
			value = m_Samples.empty() ? 1 : (double)m_Loop->GetLength() / (double)m_Samples.size();
		else return false;
		return true;
	}
}
