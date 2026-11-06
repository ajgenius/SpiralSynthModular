// SPDX-License-Identifier: GPL-2.0-or-later
#include "NoteGrid.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace Spiral::Panels;

static NoteGrid Roll()
{
	NoteGrid grid;
	grid.Width = 442;
	grid.Height = 260;
	grid.Gutter = 42;
	grid.Header = 20;
	grid.LowRow = 48;
	grid.Rows = 24;
	grid.First = 0;
	grid.Span = 4;
	grid.GridStep = 0.25;

	return grid;
}

static bool Near(double a, double b)
{
	return std::fabs(a - b) < 1e-9;
}

static GridNote Note(int id, int row, double start, double end)
{
	GridNote note;
	note.Id = id;
	note.Index = id;
	note.Row = row;
	note.Start = start;
	note.End = end;

	return note;
}

static void MapsTimeAndRows()
{
	NoteGrid grid = Roll();
	assert(Near(grid.X(0), 42) && Near(grid.X(4), 442));
	assert(Near(grid.TimeAt(242), 2));
	assert(grid.RowAt(20) == 71 && grid.RowAt(259) == 48 && grid.RowAt(10) == -1);
	assert(Near(grid.RowY(71), 20));
	assert(NoteGrid::NoteName(60) == "C4" && NoteGrid::NoteName(61) == "C#4");
	assert(NoteGrid::IsAccidental(61) && !NoteGrid::IsAccidental(60));
	std::printf("ok   maps time and rows\n");
}

static void CreatesASnappedNote()
{
	NoteGrid grid = Roll();
	const double y = grid.RowY(60) + 2;
	assert(grid.Press(grid.X(1.1), y));
	assert(grid.Mode() == NoteGrid::Create);
	GridNote made;
	assert(grid.Release(grid.X(1.1), y, made) == NoteGrid::Added);
	assert(made.Row == 60 && Near(made.Start, 1.0) && Near(made.End, 1.25));
	std::printf("ok   creates a snapped note\n");
}

static void MovesAndResizes()
{
	NoteGrid grid = Roll();
	grid.Notes.push_back(Note(7, 60, 1, 2));
	const double y = grid.RowY(60) + 2;
	assert(grid.Press(grid.X(1.5), y) && grid.Mode() == NoteGrid::Move && grid.Selected == 7);
	GridNote moved;
	assert(grid.Release(grid.X(2.5), grid.RowY(62) + 2, moved) == NoteGrid::Changed);
	assert(Near(moved.Start, 2) && Near(moved.End, 3) && moved.Row == 62 && moved.Index == 7);
	assert(grid.Press(grid.X(2), y) && grid.Mode() == NoteGrid::ResizeEnd);
	GridNote longer;
	assert(grid.Release(grid.X(3), y, longer) == NoteGrid::Changed && Near(longer.End, 3) && Near(longer.Start, 1));
	assert(grid.Press(grid.X(1), y) && grid.Mode() == NoteGrid::ResizeStart);
	GridNote later;
	assert(grid.Release(grid.X(1.5), y, later) == NoteGrid::Changed && Near(later.Start, 1.5));
	assert(grid.Press(grid.X(1.5), y));
	GridNote same;
	assert(grid.Release(grid.X(1.5), y, same) == NoteGrid::Nothing);
	std::printf("ok   moves and resizes\n");
}

static void RemovesTheSelectedNote()
{
	NoteGrid grid = Roll();
	grid.Notes.push_back(Note(3, 60, 1, 2));
	grid.Press(grid.X(1.5), grid.RowY(60) + 2);
	GridNote ignored;
	grid.Release(grid.X(1.5), grid.RowY(60) + 2, ignored);
	GridNote removed;
	assert(grid.RemoveSelected(removed) && removed.Id == 3);
	std::printf("ok   removes the selected note\n");
}

static void TogglesCellsLikeAMatrix()
{
	NoteGrid grid = Roll();
	grid.Toggle = true;
	grid.Movable = false;
	grid.VelocityDrag = true;
	grid.GridStep = 1;
	grid.Span = 16;
	grid.Snap = false;
	const double y = grid.RowY(50) + 2;
	assert(grid.Press(grid.X(3.4), y) && grid.Mode() == NoteGrid::Create);
	GridNote on;
	assert(grid.Release(grid.X(3.4), y, on) == NoteGrid::Added && Near(on.Start, 3) && Near(on.End, 4));
	grid.Notes.push_back(on);
	assert(grid.Press(grid.X(3.4), y) && grid.Mode() == NoteGrid::Level);
	GridNote off;
	assert(grid.Release(grid.X(3.4), y, off) == NoteGrid::Removed && Near(off.Start, 3));
	assert(grid.Press(grid.X(4), y) && grid.Mode() == NoteGrid::ResizeEnd);
	GridNote longer;
	assert(grid.Release(grid.X(6.2), y, longer) == NoteGrid::Changed && Near(longer.End, 7));
	assert(grid.Press(grid.X(3.4), y));
	GridNote quieter;
	assert(grid.Release(grid.X(3.4), y + 60, quieter) == NoteGrid::Changed && quieter.Velocity < on.Velocity);
	std::printf("ok   toggles cells like a matrix\n");
}

static void CancelDropsTheDrag()
{
	NoteGrid grid = Roll();
	grid.Press(grid.X(1), grid.RowY(60) + 2);
	assert(grid.Dragging());
	grid.Cancel();
	GridNote none;
	assert(!grid.Dragging() && grid.Release(grid.X(2), grid.RowY(60) + 2, none) == NoteGrid::Nothing);
	std::printf("ok   cancel drops the drag\n");
}

int main()
{
	MapsTimeAndRows();
	CreatesASnappedNote();
	MovesAndResizes();
	RemovesTheSelectedNote();
	TogglesCellsLikeAMatrix();
	CancelDropsTheDrag();

	return 0;
}
