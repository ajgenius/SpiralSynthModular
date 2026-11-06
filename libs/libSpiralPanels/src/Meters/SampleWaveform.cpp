// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
#include "ControlSupport.h"

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_SampleWaveform("SampleWaveform", &SampleWaveformControl::Create);

		PanelControl *SampleWaveformControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new SampleWaveformControl(panel, node));
		}

		SampleWaveformControl::SampleWaveformControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Display(MakeView<SampleView>("SampleWaveform", *this, node)),
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

			if (m_Display)
				Adopt(m_Display);
		}

		void SampleWaveformControl::Load()
		{
			if (m_PlayStart)
				m_Display->SetMarker(SampleView::PlayStart, (long)m_Panel.Read(*m_PlayStart));

			if (m_LoopStart)
				m_Display->SetMarker(SampleView::LoopStart, (long)m_Panel.Read(*m_LoopStart));

			if (m_LoopEnd)
				m_Display->SetMarker(SampleView::LoopEnd, (long)m_Panel.Read(*m_LoopEnd));

			m_Display->Redraw();
			MarkersChanged();
		}

		void SampleWaveformControl::Refresh()
		{
			if (!m_Fetched && m_Display->Displayed())
				Reload();

			if (m_Pos)
				m_Display->SetPlayPosition((long)m_Panel.Read(*m_Pos));
		}

		void SampleWaveformControl::Reload()
		{
			m_Fetched = true;

			if (m_SampleCommand.empty() || !m_Panel.Source())
				return;

			if (!m_Panel.Source()->FetchFloats(m_SampleCommand, m_SizeChannel, m_BufferChannel, m_Samples))
				return;

			m_Display->SetSample(m_Samples);
			m_Display->Redraw();
		}

		bool SampleWaveformControl::MarkersChanged()
		{
			const long play = m_Display->GetMarker(SampleView::PlayStart);
			const long start = m_Display->GetMarker(SampleView::LoopStart);
			const long end = m_Display->GetMarker(SampleView::LoopEnd);
			const bool changed = play != m_SentPlayStart || start != m_SentLoopStart || end != m_SentLoopEnd;
			m_SentPlayStart = play;
			m_SentLoopStart = start;
			m_SentLoopEnd = end;

			return changed;
		}

		void SampleWaveformControl::MarkerMoved()
		{
			if (MarkersChanged())
				Commit(0);
		}

		void SampleWaveformControl::SetAttribute(const std::string &attribute, double value)
		{
			if (attribute == "reload")
				Reload();
			else if (attribute == "zoom_in")
				m_Display->Zoom(true);
			else if (attribute == "zoom_out")
				m_Display->Zoom(false);
			else if (attribute == "position_marker")
				m_Display->ShowPositionMarker(value != 0);
		}

		bool SampleWaveformControl::GetAttribute(const std::string &attribute, double &value) const
		{
			if (attribute == "range_start")
				value = (double)m_Display->RangeStart();
			else if (attribute == "range_end")
				value = (double)m_Display->RangeEnd();
			else if (attribute == "play_start")
				value = (double)m_Display->GetMarker(SampleView::PlayStart);
			else if (attribute == "loop_start")
				value = (double)m_Display->GetMarker(SampleView::LoopStart);
			else if (attribute == "loop_end")
				value = (double)m_Display->GetMarker(SampleView::LoopEnd);
			else
				return false;

			return true;
		}
	}
}
