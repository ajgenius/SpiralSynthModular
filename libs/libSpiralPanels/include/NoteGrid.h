// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_NOTE_GRID_H
#define SPIRAL_PANELS_NOTE_GRID_H

#include <string>
#include <vector>

namespace Spiral
{
	namespace Panels
	{
		struct GridNote
		{
			int Id, Index, Row, Channel;
			double Start, End;
			float Velocity;
			bool Closed;
			GridNote(): Id(-1), Index(-1), Row(0), Channel(0), Start(0), End(0), Velocity(1), Closed(true) {}
		};

		class NoteGrid
		{
		public:
			enum Drag
			{
				None,
				Move,
				ResizeStart,
				ResizeEnd,
				Create,
				Level
			};

			enum Outcome
			{
				Nothing,
				Added,
				Changed,
				Removed
			};

			NoteGrid();

			double Width, Height, Gutter, Header;
			int LowRow, Rows;
			double First, Span, GridStep;
			bool Snap, Editable, Toggle, Movable, Resizable, VelocityDrag;
			int Channel;
			double Playhead;
			std::vector<GridNote> Notes;
			int Selected;

			double RowHeight() const;
			double X(double time) const;
			double TimeAt(double x) const;
			int RowAt(double y) const;
			double RowY(int row) const;
			double Snapped(double time) const;
			int HitNote(double x, double y) const;
			bool Visible(const GridNote &note) const;
			std::vector<double> GridLines(int most) const;

			bool Press(double x, double y);
			void MoveTo(double x, double y);
			Outcome Release(double x, double y, GridNote &result);
			void Cancel();
			bool Dragging() const { return m_Drag != None; }
			Drag Mode() const { return m_Drag; }
			const GridNote &Draft() const { return m_Draft; }
			const GridNote &Original() const { return m_Original; }
			bool RemoveSelected(GridNote &result) const;

			static std::string NoteName(int row);
			static bool IsAccidental(int row);

		private:
			Drag m_Drag;
			GridNote m_Original, m_Draft;
			double m_PressX, m_PressY, m_PressTime;
			int m_PressRow;
			bool m_Moved;
		};
	}
}

#endif
