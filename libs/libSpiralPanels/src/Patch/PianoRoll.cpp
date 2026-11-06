// SPDX-License-Identifier: GPL-2.0-or-later
#include "Patch.h"
#include "JSON.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "PanelSource.h"
#include <cstring>

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_PianoRoll("PianoRoll", &PianoRollControl::Create);

		PanelControl *PianoRollControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			PianoRollControl *made = new PianoRollControl(panel, node);

			if (made->View())
				return made;

			delete made;

			return NULL;
		}

		PianoRollControl::PianoRollControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Roll(NULL),
			m_Rows(TextOf(node.Get("notes"))),
			m_Count(panel.FindProperty(TextOf(node.Get("count")))),
			m_Playhead(panel.FindProperty(TextOf(node.Get("playhead"))))
		{
			m_Grid.LowRow = (int)NumberOf(node.Get("low_key"), 48);
			m_Grid.Rows = (int)NumberOf(node.Get("keys"), 24);
			m_Grid.Span = NumberOf(node.Get("span"), 4);
			m_Grid.GridStep = NumberOf(node.Get("grid"), 0.25);
			m_Grid.Gutter = NumberOf(node.Get("gutter"), 42);
			m_Grid.Header = NumberOf(node.Get("header"), 20);
			m_Grid.Snap = !node.Get("snap") || node.Get("snap")->AsBool();
			m_Grid.Width = m_W;
			m_Grid.Height = m_H;
			m_Roll = dynamic_cast<RollView *>(ViewRegistry::Get().Create("PianoRoll", *this, node));

			if (m_Roll)
				Adopt(m_Roll);
		}

		NoteGrid &PianoRollControl::Grid()
		{
			Fit();

			return m_Grid;
		}

		void PianoRollControl::Fit()
		{
			if (m_Roll)
				m_Roll->Size(m_Grid.Width, m_Grid.Height);
		}

		void PianoRollControl::Load()
		{
			PanelSource *source = m_Panel.Source();
			bool changed = false;

			if (m_Playhead)
			{
				const double playhead = m_Panel.Read(*m_Playhead);

				if (playhead != m_Grid.Playhead)
				{
					m_Grid.Playhead = playhead;
					changed = true;
				}
			}

			std::vector<unsigned char> bytes;

			if (source && !m_Rows.empty() && source->ReadBytes(m_Rows, bytes) && bytes != m_Shown && !m_Grid.Dragging())
			{
				m_Shown = bytes;
				const size_t stored = bytes.size() / sizeof(Row);
				const size_t count = m_Count ? std::min(stored, (size_t)m_Panel.Read(*m_Count)) : stored;
				m_Grid.Notes.clear();

				for (size_t i = 0; i < count; ++i)
				{
					Row row;
					std::memcpy(&row, &bytes[i * sizeof(Row)], sizeof(Row));
					GridNote note;
					note.Id = row.OnEvent;
					note.Index = row.OnEvent;
					note.Row = row.Key;
					note.Channel = row.Channel;
					note.Velocity = row.Velocity;
					note.Start = row.Start;
					note.End = row.End;
					m_Grid.Notes.push_back(note);
				}

				changed = true;
			}

			if (changed)
				m_Roll->Redraw();
		}

		void PianoRollControl::Refresh()
		{
			Load();
		}

		void PianoRollControl::SetAttribute(const std::string &attribute, double value)
		{
			if (attribute == "snap")
				m_Grid.Snap = value != 0;
			else if (attribute == "grid" && value > 0)
				m_Grid.GridStep = value;
			else if (attribute == "span" && value > 0)
				m_Grid.Span = value;
			else
				return;

			m_Roll->Redraw();
		}

		bool PianoRollControl::Press(double x, double y)
		{
			Fit();
			const bool used = m_Grid.Press(x, y);
			m_Roll->Redraw();

			return used;
		}

		void PianoRollControl::Drag(double x, double y)
		{
			if (!m_Grid.Dragging())
				return;

			Fit();
			m_Grid.MoveTo(x, y);
			m_Roll->Redraw();
		}

		void PianoRollControl::Release(double x, double y)
		{
			Fit();
			GridNote note;
			const NoteGrid::Outcome outcome = m_Grid.Release(x, y, note);
			m_Roll->Redraw();
			Edited(outcome, note);
		}

		void PianoRollControl::Cancel()
		{
			m_Grid.Cancel();
			m_Roll->Redraw();
		}

		void PianoRollControl::RemoveSelected()
		{
			GridNote note;

			if (m_Grid.RemoveSelected(note))
				Edited(NoteGrid::Removed, note);
		}

		void PianoRollControl::Scroll(int rows, double time)
		{
			m_Grid.LowRow = std::max(0, std::min(128 - m_Grid.Rows, m_Grid.LowRow + rows));
			m_Grid.First = std::max(0.0, m_Grid.First + time * m_Grid.Span);
			m_Roll->Redraw();
		}

		void PianoRollControl::Edited(NoteGrid::Outcome outcome, const GridNote &note)
		{
			Spiral::Data::Handlers::Arguments arguments;
			arguments["index"] = Spiral::Data::ControlValue((double)note.Index);
			arguments["start"] = Spiral::Data::ControlValue(note.Start);
			arguments["end"] = Spiral::Data::ControlValue(note.End);
			arguments["key"] = Spiral::Data::ControlValue((double)note.Row);
			arguments["velocity"] = Spiral::Data::ControlValue((double)note.Velocity);

			if (outcome == NoteGrid::Added)
				Fire("note_added", arguments);
			else if (outcome == NoteGrid::Changed)
				Fire("note_changed", arguments);
			else if (outcome == NoteGrid::Removed)
				Fire("note_removed", arguments);
		}

		void PianoRollControl::DeclareEvents(std::vector<Event> &out) const
		{
			static const Event added = {"note_added", {"start", "end", "key", "velocity", NULL}};
			static const Event changed = {"note_changed", {"index", "start", "end", "key", "velocity", NULL}};
			static const Event removed = {"note_removed", {"index", NULL}};
			PanelControl::DeclareEvents(out);
			out.push_back(added);
			out.push_back(changed);
			out.push_back(removed);
		}
	}
}
