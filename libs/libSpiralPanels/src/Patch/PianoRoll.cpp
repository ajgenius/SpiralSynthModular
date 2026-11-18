// SPDX-License-Identifier: GPL-2.0-or-later
#include "Patch.h"
#include "JSON.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "RowSource.h"
#include <cmath>

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
			m_Notes(TextOf(node.Get("notes"))),
			m_Playhead(panel.FindProperty(TextOf(node.Get("playhead")))),
			m_ShownCount(-1), m_ShownRevision(0),
			m_Held(false),
			m_Marks(node.Get("on") && node.Get("on")->Get("position_moved")),
			m_Marking(false)
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
			bool changed = false;

			// A marker held is where it is held, not where the device is.
			if (m_Playhead && !m_Marking)
			{
				const double playhead = m_Panel.Read(*m_Playhead);

				if (playhead != m_Grid.Playhead)
				{
					m_Grid.Playhead = playhead;

					// The page turns to keep a moving playhead in view, unless
					// a note is being drawn or a bar held.
					if (!m_Grid.Dragging() && !m_Held && playhead >= 0 && m_Grid.Span > 0
							&& (playhead < m_Grid.First || playhead >= m_Grid.First + m_Grid.Span))
						m_Grid.First = std::floor(playhead / m_Grid.Span) * m_Grid.Span;

					changed = true;
				}
			}

			// The notes are the rows of the source the panel names: a note's
			// start and end in seconds, its key and velocity, the index it is
			// named back by (its row, where the device gives none) and its
			// channel.
			Spiral::Data::RowSource *notes = m_Panel.Rows(m_Notes);

			if (notes && !m_Grid.Dragging() && (!notes->TableRevision() ||
				notes->TableRevision() != m_ShownRevision || notes->Count() != m_ShownCount))
			{
				enum { Start, End, Key, Velocity, Index, Channel, Fields };
				const int fields[Fields] = {notes->FindField("start"), notes->FindField("end"), notes->FindField("key"),
					notes->FindField("velocity"), notes->FindField("index"), notes->FindField("channel")};
				m_ShownCount = notes->Count();
				m_ShownRevision = notes->TableRevision();
				m_Grid.Notes.resize(m_ShownCount);
				for (long n = 0; n < m_ShownCount; ++n)
				{
					double values[Fields] = {0, 0, 0, 1, double(n), 0};
					for (int f = 0; f < Fields; ++f)
					{
						Spiral::Data::ControlValue cell;
						if (notes->GetAt(n, fields[f], cell))
							values[f] = cell.number;
					}
					GridNote &note = m_Grid.Notes[n];
					note.Id = note.Index = (int)values[Index];
					note.Row = (int)values[Key];
					note.Channel = (int)values[Channel];
					note.Velocity = (float)values[Velocity];
					note.Start = values[Start];
					note.End = values[End];
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
			else if (attribute == "selected_velocity")
			{
				SetSelectedVelocity((float)value);

				return;
			}
			else if (attribute == "remove_selected")
			{
				RemoveSelected();

				return;
			}
			else
				return;

			m_Roll->Redraw();
		}

		bool PianoRollControl::Press(double x, double y)
		{
			Fit();

			// A press in the strip above the rows puts the position marker
			// there, and a drag takes it along.
			if (m_Marks && y >= 0 && y < m_Grid.Header && x >= m_Grid.Gutter && x < m_Grid.Width)
			{
				m_Marking = true;
				m_Grid.Playhead = m_Grid.Snapped(m_Grid.TimeAt(x));
				m_Roll->Redraw();

				return true;
			}

			const bool used = m_Grid.Press(x, y);
			m_Roll->Redraw();

			return used;
		}

		void PianoRollControl::Drag(double x, double y)
		{
			if (!m_Grid.Dragging() && !m_Marking)
				return;

			Fit();

			// Dragged to an edge, the view moves on under the note or the
			// marker.
			if (x > m_Grid.Width - 6)
				m_Grid.First += m_Grid.Span / 32;
			else if (x < m_Grid.Gutter + 6)
				m_Grid.First = std::max(0.0, m_Grid.First - m_Grid.Span / 32);

			if (m_Marking)
				m_Grid.Playhead = m_Grid.Snapped(m_Grid.TimeAt(std::max(m_Grid.Gutter, std::min(m_Grid.Width, x))));
			else
				m_Grid.MoveTo(x, y);

			m_Roll->Redraw();
		}

		void PianoRollControl::Release(double x, double y)
		{
			// The marker let go: the device is told where.
			if (m_Marking)
			{
				Drag(x, y);
				m_Marking = false;

				Spiral::Data::Handlers::Arguments arguments;
				arguments["position"] = Spiral::Data::ControlValue(m_Grid.Playhead);
				Fire("position_moved", arguments);

				return;
			}

			Fit();
			GridNote note;
			const NoteGrid::Outcome outcome = m_Grid.Release(x, y, note);
			m_Roll->Redraw();
			Edited(outcome, note);
		}

		void PianoRollControl::Cancel()
		{
			m_Marking = false;
			m_Grid.Cancel();
			m_Roll->Redraw();
		}

		void PianoRollControl::RemoveSelected()
		{
			GridNote note;

			if (m_Grid.RemoveSelected(note))
				Edited(NoteGrid::Removed, note);
		}

		void PianoRollControl::SetSelectedVelocity(float velocity)
		{
			if (m_Grid.Dragging())
				return;

			for (size_t i = 0; i < m_Grid.Notes.size(); ++i)
			{
				if (m_Grid.Notes[i].Id != m_Grid.Selected)
					continue;

				GridNote changed = m_Grid.Notes[i];
				changed.Velocity = std::max(0.0f, std::min(1.0f, velocity));
				Edited(NoteGrid::Changed, changed);

				return;
			}
		}

		void PianoRollControl::ShowFrom(int lowRow, double first)
		{
			m_Grid.LowRow = std::max(0, std::min(128 - m_Grid.Rows, lowRow));
			m_Grid.First = std::max(0.0, first);
			m_Roll->Redraw();
		}

		// As far as the notes or the playhead go, never less than a page nor
		// than where the view already is.
		double PianoRollControl::TimeEnd() const
		{
			double last = std::max(0.0, m_Grid.Playhead);

			for (size_t i = 0; i < m_Grid.Notes.size(); ++i)
				last = std::max(last, m_Grid.Notes[i].End);

			return std::max(m_Grid.First, std::max(last, m_Grid.Span));
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
			static const Event moved = {"position_moved", {"position", NULL}};
			PanelControl::DeclareEvents(out);
			out.push_back(added);
			out.push_back(changed);
			out.push_back(removed);
			out.push_back(moved);
		}
	}
}
