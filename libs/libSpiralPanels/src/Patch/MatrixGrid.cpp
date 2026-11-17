// SPDX-License-Identifier: GPL-2.0-or-later
#include "Patch.h"
#include "JSON.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelSource.h"
#include "PanelProperty.h"
#include "RowSource.h"
#include <algorithm>
#include <climits>
#include <cstring>

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_MatrixGrid("MatrixGrid", &MatrixGridControl::Create);

		static int IntAt(const std::vector<unsigned char> &bytes)
		{
			int value = 0;

			if (bytes.size() >= sizeof(int))
				std::memcpy(&value, &bytes[0], sizeof(value));

			return value;
		}

		PanelControl *MatrixGridControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			MatrixGridControl *made = new MatrixGridControl(panel, node);

			if (made->View())
				return made;

			delete made;

			return NULL;
		}

		MatrixGridControl::MatrixGridControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_View(NULL),
			m_Notes(TextOf(node.Get("notes"))),
			m_StepChannel(TextOf(node.Get("step"))),
			m_TriggerChannel(TextOf(node.Get("triggers"))),
			m_Octave(panel.FindProperty(TextOf(node.Get("octave")))),
			m_Columns(std::max(1, (int)NumberOf(node.Get("columns"), 16))),
			m_Rows(std::max(1, (int)NumberOf(node.Get("rows"), 8))),
			m_Step(-1),
			m_TriggerRow(INT_MIN),
			m_Marking(false)
		{
			m_Grid.LowRow = 0;
			m_Grid.Rows = m_Rows;
			m_Grid.First = 0;
			m_Grid.Span = m_Columns;
			m_Grid.GridStep = 1;
			m_Grid.Snap = false;
			m_Grid.Toggle = true;
			m_Grid.Movable = false;
			m_Grid.Header = 0;
			m_NoteNames = NumberOf(node.Get("note_names"), 0);
			m_Grid.Gutter = m_NoteNames;
			m_Grid.Width = m_W;
			m_Grid.Height = m_H;
			m_View = dynamic_cast<RollView *>(ViewRegistry::Get().Create("MatrixGrid", *this, node));

			if (m_View)
				Adopt(m_View);
		}

		void MatrixGridControl::Fit()
		{
			if (!m_View)
				return;

			m_View->Size(m_Grid.Width, m_Grid.Height);
			m_Grid.Gutter = m_H > 0 ? m_NoteNames * m_Grid.Height / m_H : m_NoteNames;

			m_Grid.Resizable = HasResize();
			m_Grid.VelocityDrag = HasVolume();
		}

		NoteGrid &MatrixGridControl::Grid()
		{
			Fit();

			return m_Grid;
		}

		bool MatrixGridControl::On(int column, int row) const
		{
			const size_t i = (size_t)column * m_Rows + row;

			return column >= 0 && row >= 0 && row < m_Rows && i < m_Cells.size() && m_Cells[i];
		}

		int MatrixGridControl::EndOf(int column, int row) const
		{
			const size_t i = (size_t)column * m_Rows + row;
			const int end = i < m_Ends.size() ? std::max(column + 1, std::min(m_Columns, (int)m_Ends[i])) : column + 1;

			for (int next = column + 1; next < end; ++next)
			{
				if (On(next, row))
					return next;
			}

			return end;
		}

		int MatrixGridControl::NoteAt(int column, int row) const
		{
			for (int start = column; start >= 0; --start)
			{
				if (On(start, row))
					return EndOf(start, row) > column ? start : -1;
			}

			return -1;
		}

		float MatrixGridControl::VolumeAt(int column, int row) const
		{
			const size_t i = ((size_t)column * m_Rows + row) * sizeof(float);
			float volume = 1.0f;

			if (i + sizeof(float) <= m_Volumes.size())
				std::memcpy(&volume, &m_Volumes[i], sizeof(volume));

			return volume;
		}

		void MatrixGridControl::SetCell(int column, int row, bool on)
		{
			const size_t i = (size_t)column * m_Rows + row;

			if (i < m_Cells.size())
				m_Cells[i] = on;

			if (i < m_Ends.size())
				m_Ends[i] = (unsigned char)(column + 1);
		}

		void MatrixGridControl::SetEnd(int column, int row, int end)
		{
			const size_t i = (size_t)column * m_Rows + row;

			if (i < m_Ends.size())
				m_Ends[i] = (unsigned char)end;
		}

		void MatrixGridControl::SetVolume(int column, int row, float volume)
		{
			const size_t i = ((size_t)column * m_Rows + row) * sizeof(float);

			if (i + sizeof(float) <= m_Volumes.size())
				std::memcpy(&m_Volumes[i], &volume, sizeof(volume));
		}

		void MatrixGridControl::Rebuild()
		{
			m_Grid.Notes.clear();

			for (int row = 0; row < m_Rows; ++row)
			{
				for (int column = 0; column < m_Columns; ++column)
				{
					if (!On(column, row))
						continue;

					GridNote note;
					note.Id = column * m_Rows + row;
					note.Index = note.Id;
					note.Row = row;
					note.Start = column;
					note.End = EndOf(column, row);
					note.Velocity = m_Volumes.empty() ? 1.0f : VolumeAt(column, row);
					m_Grid.Notes.push_back(note);
				}
			}
		}

		// The grid is drawn from the notes of the pattern on show: the rows
		// of the source it names, each a note's start step, the step it ends
		// before, its key and its velocity. The grid's bottom row is the
		// pattern's octave, so a note of another octave has no cell here.
		void MatrixGridControl::Load()
		{
			Spiral::Data::RowSource *notes = m_Panel.Rows(m_Notes);

			if (!notes || m_Grid.Dragging())
				return;

			const int start = notes->FindField("start"), end = notes->FindField("end");
			const int key = notes->FindField("key"), velocity = notes->FindField("velocity");
			const int low = m_Octave ? (int)m_Panel.Read(*m_Octave) * 12 : 0;
			const size_t size = (size_t)m_Columns * m_Rows;
			const float full = 1.0f;
			std::vector<unsigned char> cells(size, 0), ends(size, 0), volumes(size * sizeof(float));

			for (size_t i = 0; i < size; ++i)
				std::memcpy(&volumes[i * sizeof(float)], &full, sizeof(full));

			for (long n = 0; n < notes->Count(); ++n)
			{
				Spiral::Data::ControlValue at, to, pitch, level;

				if (!notes->GetAt(n, start, at) || !notes->GetAt(n, key, pitch))
					continue;

				const int column = (int)at.number, row = (int)pitch.number - low;

				if (column < 0 || column >= m_Columns || row < 0 || row >= m_Rows)
					continue;

				const size_t i = (size_t)column * m_Rows + row;
				const float volume = notes->GetAt(n, velocity, level) ? (float)level.number : full;

				cells[i] = 1;
				ends[i] = (unsigned char)(notes->GetAt(n, end, to) ? (int)to.number : column + 1);
				std::memcpy(&volumes[i * sizeof(float)], &volume, sizeof(volume));
			}

			if (cells == m_Cells && ends == m_Ends && volumes == m_Volumes)
				return;

			m_Cells.swap(cells);
			m_Ends.swap(ends);
			m_Volumes.swap(volumes);
			Rebuild();
			m_View->Redraw();
		}

		void MatrixGridControl::Refresh()
		{
			Load();
			PanelSource *source = m_Panel.Source();

			if (!source)
				return;

			std::vector<unsigned char> step, row;

			if (source->ReadBytes(m_StepChannel, step) && step.size() >= sizeof(int) && IntAt(step) != m_Step)
			{
				m_Step = IntAt(step);
				m_Grid.Playhead = m_Step;
				m_View->Redraw();
			}

			if (!m_TriggerChannel.empty() && !m_Marking && source->ReadBytes(m_TriggerChannel, row)
					&& row.size() >= sizeof(int) && IntAt(row) != m_TriggerRow)
			{
				m_TriggerRow = IntAt(row);
				m_View->Redraw();
			}
		}

		bool MatrixGridControl::Press(double x, double y)
		{
			Fit();

			if (!m_Grid.Press(x, y))
				return false;

			const GridNote &draft = m_Grid.Draft();
			m_Applied = draft;

			if (m_Grid.Mode() == NoteGrid::Create)
			{
				SetCell((int)draft.Start, draft.Row, true);
				Act((int)draft.Start, draft.Row, 1);
				Rebuild();
			}

			m_View->Redraw();

			return true;
		}

		void MatrixGridControl::Drag(double x, double y)
		{
			if (!m_Grid.Dragging())
				return;

			Fit();
			m_Grid.MoveTo(x, y);
			GridNote draft = m_Grid.Draft();
			const int column = (int)draft.Start;
			const NoteGrid::Drag mode = m_Grid.Mode();

			if ((mode == NoteGrid::Create || mode == NoteGrid::ResizeEnd) && HasResize())
			{
				int end = std::max(column + 1, std::min(m_Columns, (int)draft.End));

				for (int next = column + 1; next < end; ++next)
				{
					if (On(next, draft.Row))
					{
						end = next;

						break;
					}
				}

				if (end != (int)m_Applied.End)
				{
					SetEnd(column, draft.Row, end);
					Resize(column, draft.Row, end);
					m_Applied.End = end;
					Rebuild();
				}
			}
			else if (mode == NoteGrid::Level && HasVolume() && draft.Velocity != m_Applied.Velocity)
			{
				SetVolume(column, draft.Row, draft.Velocity);
				Volume(column, draft.Row, draft.Velocity);
				m_Applied.Velocity = draft.Velocity;
				Rebuild();
			}

			m_View->Redraw();
		}

		void MatrixGridControl::Release(double x, double y)
		{
			Drag(x, y);
			GridNote note;

			if (m_Grid.Release(x, y, note) == NoteGrid::Removed)
			{
				SetCell((int)note.Start, note.Row, false);
				Act((int)note.Start, note.Row, 0);
				Rebuild();
			}

			m_View->Redraw();
		}

		void MatrixGridControl::Wheel(double x, double y, int steps)
		{
			Fit();
			const int hit = m_Grid.HitNote(x, y);

			if (hit < 0 || !HasVolume())
				return;

			const GridNote &note = m_Grid.Notes[hit];
			SetVolumeAt((int)note.Start, note.Row, note.Velocity - steps * 0.05f);
		}

		void MatrixGridControl::SetVolumeAt(int column, int row, float volume)
		{
			if (!HasVolume() || !On(column, row))
				return;

			volume = std::max(0.0f, std::min(1.0f, volume));
			SetVolume(column, row, volume);
			Volume(column, row, volume);
			Rebuild();
			m_View->Redraw();
		}

		void MatrixGridControl::StartMarking(int row)
		{
			m_Marking = true;
			MarkTo(row);
		}

		void MatrixGridControl::MarkTo(int row)
		{
			row = std::max(0, std::min(m_Rows - 1, row));

			if (row == m_TriggerRow)
				return;

			m_TriggerRow = row;
			MoveTriggers(row);
			m_View->Redraw();
		}

		void MatrixGridControl::EndMarking()
		{
			m_Marking = false;
		}

		void MatrixGridControl::Act(int x, int y, int on)
		{
			Spiral::Data::Handlers::Arguments arguments;
			arguments["x"] = Spiral::Data::ControlValue((double)x);
			arguments["y"] = Spiral::Data::ControlValue((double)y);
			Fire(on ? "activate" : "deactivate", arguments);
		}

		bool MatrixGridControl::HasVolume() const
		{
			return Handles("volume");
		}

		void MatrixGridControl::Volume(int x, int y, float volume)
		{
			if (!HasVolume())
				return;

			Spiral::Data::Handlers::Arguments arguments;
			arguments["x"] = Spiral::Data::ControlValue((double)x);
			arguments["y"] = Spiral::Data::ControlValue((double)y);
			arguments["volume"] = Spiral::Data::ControlValue((double)volume);
			Fire("volume", arguments);
		}

		bool MatrixGridControl::HasResize() const
		{
			return Handles("resize");
		}

		void MatrixGridControl::Resize(int x, int y, int end)
		{
			if (!HasResize())
				return;

			Spiral::Data::Handlers::Arguments arguments;
			arguments["x"] = Spiral::Data::ControlValue((double)x);
			arguments["y"] = Spiral::Data::ControlValue((double)y);
			arguments["end"] = Spiral::Data::ControlValue((double)end);
			Fire("resize", arguments);
		}

		bool MatrixGridControl::HasTriggers() const
		{
			return Handles("triggers");
		}

		void MatrixGridControl::MoveTriggers(int y)
		{
			if (!HasTriggers())
				return;

			Spiral::Data::Handlers::Arguments arguments;
			arguments["y"] = Spiral::Data::ControlValue((double)y);
			Fire("triggers", arguments);
		}

		void MatrixGridControl::DeclareEvents(std::vector<Event> &out) const
		{
			static const Event activate = {"activate", {"x", "y", NULL}};
			static const Event deactivate = {"deactivate", {"x", "y", NULL}};
			static const Event volume = {"volume", {"x", "y", "volume", NULL}};
			static const Event resize = {"resize", {"x", "y", "end", NULL}};
			static const Event triggers = {"triggers", {"y", NULL}};
			PanelControl::DeclareEvents(out);
			out.push_back(activate);
			out.push_back(deactivate);
			out.push_back(volume);
			out.push_back(resize);
			out.push_back(triggers);
		}
	}
}
