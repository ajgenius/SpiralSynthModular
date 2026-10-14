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
#include "PanelSampleWaveform.h"
#include "Panel.h"
#include "DeviceSource.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include "Fl_WaveDisplay.H"
#include <FL/Fl.H>
#include <FL/fl_draw.H>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("SampleWaveform", &PanelSampleWaveform::Create);

	PanelControl *PanelSampleWaveform::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelSampleWaveform(panel, node);
	}

	PanelSampleWaveform::PanelSampleWaveform(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_Pos(panel.FindProperty(TextOf(node.Get("pos")))),
		m_PlayStart(panel.FindProperty(TextOf(node.Get("play_start")))),
		m_LoopStart(panel.FindProperty(TextOf(node.Get("loop_start")))),
		m_LoopEnd(panel.FindProperty(TextOf(node.Get("loop_end")))),
		m_Fetched(false),
		m_SentPlayStart(-1),
		m_SentLoopStart(-1),
		m_SentLoopEnd(-1)
	{
		const Spumoni::JSON *sample = node.Get("sample");
		if (sample)
		{
			m_SampleCommand = TextOf(sample->Get("command"));
			m_SizeChannel = TextOf(sample->Get("size"));
			m_BufferChannel = TextOf(sample->Get("buffer"));
		}
		m_Display = new Fl_WaveDisplay(m_X, m_Y, m_W, m_H, "");
		m_Display->SetColours(fl_rgb_color(20, 60, 20),
			fl_rgb_color(100, 200, 100),
			FL_WHITE,
			fl_rgb_color(203, 255, 0),
			fl_rgb_color(155, 155, 50));
		m_Display->callback(MarkerMoved, this);
		Adopt(m_Display);
	}

	void PanelSampleWaveform::Load()
	{
		if (m_PlayStart)
			m_Display->SetPlayStart((long)m_Panel.Read(*m_PlayStart));
		if (m_LoopStart)
			m_Display->SetLoopStart((long)m_Panel.Read(*m_LoopStart));
		if (m_LoopEnd)
			m_Display->SetLoopEnd((long)m_Panel.Read(*m_LoopEnd));
		m_Display->redraw();
		MarkersChanged();
	}

	void PanelSampleWaveform::Refresh()
	{
		if (!m_Fetched && m_Display->visible())
			Reload();
		if (m_Pos)
			m_Display->SetPlayPos((long)m_Panel.Read(*m_Pos));
	}

	void PanelSampleWaveform::Reload()
	{
		m_Fetched = true;
		if (m_SampleCommand.empty() || !m_Panel.Source())
			return;
		if (!m_Panel.Source()->FetchFloats(m_SampleCommand, m_SizeChannel, m_BufferChannel, m_Samples))
			return;
		if (m_Samples.empty())
			m_Display->ClearSample();
		else
			m_Display->SetSample(&m_Samples[0], (long)m_Samples.size());
		m_Display->redraw();
	}

	bool PanelSampleWaveform::MarkersChanged()
	{
		const long play = m_Display->GetPlayStart(), start = m_Display->GetLoopStart(), end = m_Display->GetLoopEnd();
		const bool changed = play != m_SentPlayStart || start != m_SentLoopStart || end != m_SentLoopEnd;
		m_SentPlayStart = play;
		m_SentLoopStart = start;
		m_SentLoopEnd = end;
		return changed;
	}

	void PanelSampleWaveform::MarkerMoved(Fl_Widget *, void *data)
	{
		PanelSampleWaveform *self = (PanelSampleWaveform *)data;
		if (self->MarkersChanged())
			self->Commit(0);
	}

	void PanelSampleWaveform::SetAttribute(const std::string &attribute, double value)
	{
		if (attribute == "reload")
			Reload();
		else if (attribute == "zoom_in")
			m_Display->ZoomIn();
		else if (attribute == "zoom_out")
			m_Display->ZoomOut();
		else if (attribute == "position_marker")
			m_Display->SetPosMarker(value != 0);
	}

	bool PanelSampleWaveform::GetAttribute(const std::string &attribute, double &value) const
	{
		if (attribute == "range_start") value = (double)m_Display->GetRangeStart();
		else if (attribute == "range_end") value = (double)m_Display->GetRangeEnd();
		else if (attribute == "play_start") value = (double)m_Display->GetPlayStart();
		else if (attribute == "loop_start") value = (double)m_Display->GetLoopStart();
		else if (attribute == "loop_end") value = (double)m_Display->GetLoopEnd();
		else return false;
		return true;
	}
}
