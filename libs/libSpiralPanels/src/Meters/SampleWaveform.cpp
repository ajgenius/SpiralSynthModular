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
			m_Pos(NULL),
			m_PlayStart(NULL),
			m_LoopStart(NULL),
			m_LoopEnd(NULL),
			m_PosName(TextOf(node.Get("pos"))),
			m_PlayStartName(TextOf(node.Get("play_start"))),
			m_LoopStartName(TextOf(node.Get("loop_start"))),
			m_LoopEndName(TextOf(node.Get("loop_end"))),
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

			FindMarks();

			if (m_Display)
				Adopt(m_Display);
		}

		// The properties the marks are kept in: as named, or on a row source
		// the row's, the name with the row's number after it.
		void SampleWaveformControl::FindMarks()
		{
			std::string number;

			if (!RowSourceName().empty() && m_Panel.Rows(RowSourceName()))
				number = TextOfNumber((double)m_Panel.RowNumber(RowSourceName()));

			m_Pos = m_PosName.empty() ? NULL : m_Panel.FindProperty(m_PosName + number);
			m_PlayStart = m_PlayStartName.empty() ? NULL : m_Panel.FindProperty(m_PlayStartName + number);
			m_LoopStart = m_LoopStartName.empty() ? NULL : m_Panel.FindProperty(m_LoopStartName + number);
			m_LoopEnd = m_LoopEndName.empty() ? NULL : m_Panel.FindProperty(m_LoopEndName + number);
		}

		// Another row's sample is on show: its marks, and the sample itself.
		void SampleWaveformControl::RowMoved()
		{
			FindMarks();
			m_SentPlayStart = m_SentLoopStart = m_SentLoopEnd = -1;
			Reload();
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
