// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_PATCH_H
#define SPIRAL_PANELS_PATCH_H

#include "Controls.h"
#include "NoteGrid.h"
#include <string>
#include <vector>

namespace Spiral
{
	namespace Panels
	{
		class PortsView: public virtual ControlView
		{
		public:
			virtual void SetPortCount(int column, int count, const std::string &labelFormat) = 0;
			virtual int PortCount(int column) const = 0;
			virtual void SetPort(int column, int index, const std::string &name, bool connected) = 0;
		};

		class KeysView: public virtual ControlView
		{
		public:
			virtual void SetKey(int index, bool down) = 0;
			virtual void Frame(bool focused, int firstNote, int lastNote) = 0;
			virtual bool Focused() const = 0;
			virtual void ShowNote(int note) = 0;
		};

		class RollView: public virtual ControlView
		{
		public:
			virtual void Redraw() = 0;
			virtual void Size(double &w, double &h) const = 0;
		};

		class PianoRollControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void Refresh();
			virtual void SetAttribute(const std::string &attribute, double value);
			void DeclareEvents(std::vector<Event> &out) const;

			NoteGrid &Grid();
			bool Press(double x, double y);
			void Drag(double x, double y);
			void Release(double x, double y);
			void Cancel();
			void RemoveSelected();
			void SetSelectedVelocity(float velocity);
			void Scroll(int rows, double time);

			// For the view's scroll bars: where the view is put, how far
			// the time bar reaches, and a bar held so the page stays put.
			void ShowFrom(int lowRow, double first);
			double TimeEnd() const;
			void HoldView(bool held) { m_Held = held; }

			// Whether the position marker is moved by hand: where the
			// panel says what that does.
			bool Marks() const { return m_Marks; }

		protected:
			PianoRollControl(Panel &panel, const Spumoni::JSON &node);
			void Fit();
			void Edited(NoteGrid::Outcome outcome, const GridNote &note);

			RollView *m_Roll;
			NoteGrid m_Grid;
			std::string m_Notes;
			const PanelProperty *m_Playhead;
			long m_ShownCount;
			unsigned long m_ShownRevision;
			bool m_Held, m_Marks, m_Marking;
		};

		class PortListControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void Refresh();
			void Clicked(int column, int index, bool pressed);
			int PortNumber(int index) const { return m_Query.empty() ? index : index + 1; }

		protected:
			struct Column
			{
				const PanelProperty *Count, *NamesCount;
				std::string Label, Connections, Names, Connect, Disconnect;
				std::vector<unsigned char> Shown;
				Column(): Count(NULL), NamesCount(NULL) {}
			};

			PortListControl(Panel &panel, const Spumoni::JSON &node);
			void Read(Column &column, const Spumoni::JSON *node);
			void Sync(int index);
			void Show(int index);

			PortsView *m_Ports;
			Column m_Columns[2];
			const PanelProperty *m_Connected;
			std::string m_NamesCommand, m_RefreshCommand;
			std::string m_Routes, m_Query, m_Add, m_Remove;
			int m_PendingPort;
			unsigned m_Ticket;
			bool m_Choosing;
			void ReceiveRoutes();
		};

		class PianoKeysControl: public PanelControl
		{
		public:
			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);
			static bool IsBlack(int note);
			static const char *KeyMap();
			static int KeyMapSize();

			virtual void Load();
			virtual void Refresh();
			void KeyChanged(int index, bool down);
			bool ComputerKeys() const { return m_ComputerKeys; }
			bool Toggles() const { return m_Toggle; }
			int First() const { return m_First; }
			int Count() const { return m_Count; }
			int Octave() const { return m_Octave; }
			int LastMapped() const;
			void Entered();
			void Unfocused();
			void SelectOctave(int octave);
			bool Typed(char key, bool down);

		protected:
			PianoKeysControl(Panel &panel, const Spumoni::JSON &node);
			void RefreshFocus();
			void Act(bool on, int note);
			void Press(int note, bool down);
			void ReleaseKeys();

			KeysView *m_Keys;
			std::string m_On, m_Off;
			int m_First, m_Count;
			bool m_Toggle;
			std::vector<const PanelProperty *> m_Notes;
			bool m_Inverted;
			bool m_ComputerKeys;
			int m_Octave;
			std::vector<int> m_Held;
			std::string m_Status, m_StatusText;
			bool m_Framed;
			int m_FramedOctave;
		};

		class MatrixGridControl: public PanelControl
		{
		public:
			enum
			{
				Triggers = 16
			};

			static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

			virtual void Load();
			virtual void Refresh();
			void DeclareEvents(std::vector<Event> &out) const;

			NoteGrid &Grid();
			int Columns() const { return m_Columns; }
			int Rows() const { return m_Rows; }
			int Step() const { return m_Step; }
			int TriggerRow() const { return m_TriggerRow; }
			bool Marking() const { return m_Marking; }
			bool HasVolume() const;
			bool HasResize() const;
			bool HasTriggers() const;
			int NoteAt(int column, int row) const;
			int EndOf(int column, int row) const;
			float VolumeAt(int column, int row) const;

			bool Press(double x, double y);
			void Drag(double x, double y);
			void Release(double x, double y);
			void Wheel(double x, double y, int steps);
			void SetVolumeAt(int column, int row, float volume);
			void StartMarking(int row);
			void MarkTo(int row);
			void EndMarking();

		protected:
			MatrixGridControl(Panel &panel, const Spumoni::JSON &node);
			void Fit();
			bool On(int column, int row) const;
			void SetCell(int column, int row, bool on);
			void SetEnd(int column, int row, int end);
			void SetVolume(int column, int row, float volume);
			void Rebuild();
			void Act(int x, int y, int on);
			void Volume(int x, int y, float volume);
			void Resize(int x, int y, int end);
			void MoveTriggers(int y);

			RollView *m_View;
			NoteGrid m_Grid;
			std::string m_Notes, m_StepChannel, m_TriggerChannel;
			const PanelProperty *m_Octave;
			int m_Columns, m_Rows, m_Step, m_TriggerRow;
			double m_NoteNames;
			bool m_Marking;
			std::vector<unsigned char> m_Cells, m_Volumes, m_Ends;
			GridNote m_Applied;
		};
	}
}

#endif
