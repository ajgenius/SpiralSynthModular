// SPDX-License-Identifier: GPL-2.0-or-later
#include "NoteGrid.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Spiral
{
	namespace Panels
	{
		static const double EdgeGrab = 4;
		static const double MinWidth = 3;

		NoteGrid::NoteGrid():
			Width(0),
			Height(0),
			Gutter(42),
			Header(20),
			LowRow(48),
			Rows(24),
			First(0),
			Span(1),
			GridStep(0.25),
			Snap(true),
			Editable(true),
			Toggle(false),
			Movable(true),
			Resizable(true),
			VelocityDrag(false),
			Playhead(-1),
			Selected(-1),
			m_Drag(None),
			m_PressX(0),
			m_PressY(0),
			m_PressTime(0),
			m_PressRow(-1),
			m_Moved(false)
		{
		}

		double NoteGrid::RowHeight() const
		{
			return std::max(1.0, Height - Header) / std::max(1, Rows);
		}

		double NoteGrid::X(double time) const
		{
			return Gutter + (time - First) * std::max(1.0, Width - Gutter) / (Span > 0 ? Span : 1);
		}

		double NoteGrid::TimeAt(double x) const
		{
			const double time = First + (x - Gutter) * (Span > 0 ? Span : 1) / std::max(1.0, Width - Gutter);

			return time < 0 ? 0 : time;
		}

		int NoteGrid::RowAt(double y) const
		{
			if (y < Header || y >= Height)
				return -1;

			return LowRow + Rows - 1 - (int)((y - Header) / RowHeight());
		}

		double NoteGrid::RowY(int row) const
		{
			return Header + (LowRow + Rows - 1 - row) * RowHeight();
		}

		double NoteGrid::Snapped(double time) const
		{
			if (!Snap || GridStep <= 0)
				return time;

			const double snapped = std::floor(time / GridStep + 0.5) * GridStep;

			return snapped < 0 ? 0 : snapped;
		}

		// A note is shown whatever channel it is on: the grid has notes, not
		// channels.
		bool NoteGrid::Visible(const GridNote &note) const
		{
			return note.Row >= LowRow && note.Row < LowRow + Rows && note.End >= First && note.Start <= First + Span;
		}

		int NoteGrid::HitNote(double x, double y) const
		{
			if (x < Gutter || x >= Width)
				return -1;

			const int row = RowAt(y);

			for (int i = (int)Notes.size() - 1; i >= 0; --i)
			{
				const GridNote &note = Notes[i];

				if (note.Row == row && x >= X(note.Start) && x <= std::max(X(note.Start) + MinWidth, X(note.End)))
					return i;
			}

			return -1;
		}

		std::vector<double> NoteGrid::GridLines(int most) const
		{
			std::vector<double> lines;

			if (GridStep <= 0 || Span <= 0)
				return lines;

			const double stride = std::max(1.0, std::ceil(Span / GridStep / std::max(1, most)));
			const double spacing = GridStep * stride;

			for (double time = std::ceil(First / spacing) * spacing; time <= First + Span; time += spacing)
				lines.push_back(time);

			return lines;
		}

		bool NoteGrid::Press(double x, double y)
		{
			m_Drag = None;
			m_Moved = false;

			if (x < Gutter || x >= Width || RowAt(y) < 0)
				return false;

			const int hit = HitNote(x, y);
			m_PressX = x;
			m_PressY = y;
			m_PressTime = TimeAt(x);
			m_PressRow = RowAt(y);

			if (hit >= 0)
			{
				m_Original = m_Draft = Notes[hit];
				Selected = m_Original.Id;

				if (!Editable || !m_Original.Closed)
					return true;

				if (Resizable && std::fabs(x - X(m_Original.End)) <= EdgeGrab)
					m_Drag = ResizeEnd;
				else if (Resizable && Movable && std::fabs(x - X(m_Original.Start)) <= EdgeGrab)
					m_Drag = ResizeStart;
				else if (VelocityDrag)
					m_Drag = Level;
				else
					m_Drag = Movable ? Move : Level;

				return true;
			}

			if (!Editable)
				return true;

			m_Draft = GridNote();
			m_Draft.Row = m_PressRow;
			m_Draft.Velocity = Toggle ? 1.0f : 100 / 127.0f;
			m_Draft.Start = Toggle ? std::floor(TimeAt(x) / GridStep) * GridStep : Snapped(TimeAt(x));
			m_Draft.End = m_Draft.Start + (GridStep > 0 ? GridStep : Span / 16);
			m_Original = m_Draft;
			m_Drag = Create;

			return true;
		}

		void NoteGrid::MoveTo(double x, double y)
		{
			if (m_Drag == None)
				return;

			if (std::fabs(x - m_PressX) < 1 && std::fabs(y - m_PressY) < 1)
			{
				m_Draft = m_Original;

				return;
			}

			m_Moved = true;
			const double time = TimeAt(x);

			if (m_Drag == Move)
			{
				const double duration = m_Original.End - m_Original.Start;
				m_Draft.Start = Snapped(std::max(0.0, m_Original.Start + time - m_PressTime));
				m_Draft.End = m_Draft.Start + duration;
				const int row = RowAt(y);

				if (row >= 0)
					m_Draft.Row = std::max(0, std::min(127, m_Original.Row + row - m_PressRow));
			}
			else if (m_Drag == ResizeStart)
				m_Draft.Start = std::min(m_Draft.End, Snapped(time));
			else if (m_Drag == ResizeEnd || (m_Drag == Create && (!Toggle || Resizable)))
			{
				double end = Toggle ? std::ceil(time / GridStep) * GridStep : Snapped(time);
				m_Draft.End = std::max(m_Draft.Start + (Toggle ? GridStep : 0), end);
			}
			else if (m_Drag == Level)
			{
				const double level = m_Original.Velocity + (m_PressY - y) / std::max(1.0, Height - Header);
				m_Draft.Velocity = (float)std::max(0.01, std::min(1.0, level));
			}
		}

		NoteGrid::Outcome NoteGrid::Release(double x, double y, GridNote &result)
		{
			if (m_Drag == None)
				return Nothing;

			MoveTo(x, y);
			const Drag drag = m_Drag;
			result = m_Draft;
			m_Drag = None;

			if (drag == Create)
				return Added;

			if (Toggle && !m_Moved && drag != ResizeEnd && drag != ResizeStart)
			{
				result = m_Original;

				return Removed;
			}

			const bool changed = result.Start != m_Original.Start || result.End != m_Original.End
					|| result.Row != m_Original.Row || result.Velocity != m_Original.Velocity;

			return changed ? Changed : Nothing;
		}

		void NoteGrid::Cancel()
		{
			m_Drag = None;
		}

		bool NoteGrid::RemoveSelected(GridNote &result) const
		{
			if (!Editable || m_Drag != None)
				return false;

			for (size_t i = 0; i < Notes.size(); ++i)
			{
				if (Notes[i].Id == Selected)
				{
					result = Notes[i];

					return true;
				}
			}

			return false;
		}

		std::string NoteGrid::NoteName(int row)
		{
			static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
			char text[8];
			std::snprintf(text, sizeof(text), "%s%d", names[((row % 12) + 12) % 12], row / 12 - 1);

			return text;
		}

		bool NoteGrid::IsAccidental(int row)
		{
			const int n = ((row % 12) + 12) % 12;

			return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
		}
	}
}
