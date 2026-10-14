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
#include "PanelPianoKeys.h"
#include "Panel.h"
#include "DeviceSource.h"
#include "PanelControlRegistry.h"
#include "JSON.h"
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input_.H>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("PianoKeys", &PanelPianoKeys::Create);

	static bool IsBlack(int note)
	{
		const int n = note % 12;
		return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
	}

	// The old Keyboard editor's computer keys, in SoundTracker order: the bottom row
	// is the chosen octave, the top row the octave above.
	static const char ComputerKeys[] = "zsxdcvgbhnjmq2w3er5t6y7ui9o0p[";
	static const int ComputerKeyCount = sizeof(ComputerKeys) - 1;

	// The keys' group, which hands its events to the control so it can play the computer keyboard.
	class PianoKeysGroup: public Fl_Group
	{
	public:
		PianoKeysGroup(int x, int y, int w, int h, PanelPianoKeys &owner):
			Fl_Group(x, y, w, h), m_Owner(owner) {}

		virtual int handle(int event)
		{
			const int used = Fl_Group::handle(event);
			return m_Owner.Typed(event) || used;
		}

		// A white key drawn again alone covers the black keys over its
		// edges, which come after it: draw those again on top of it.
		virtual void draw()
		{
			if (!(damage() & ~FL_DAMAGE_CHILD))
				for (int i = 0; i < children(); ++i)
				{
					const Fl_Widget *white = child(i);
					if (!white->damage())
						continue;
					for (int j = i + 1; j < children(); ++j)
					{
						Fl_Widget *black = child(j);
						if (black->x() < white->x() + white->w() && white->x() < black->x() + black->w() &&
						    black->y() < white->y() + white->h() && white->y() < black->y() + black->h())
							black->clear_damage(black->damage() | FL_DAMAGE_ALL);
					}
				}
			Fl_Group::draw();
		}

	private:
		PanelPianoKeys &m_Owner;
	};

	PanelControl *PanelPianoKeys::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelPianoKeys(panel, node);
	}

	PanelPianoKeys::PanelPianoKeys(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_On(TextOf(node.Get("note_on"))),
		m_Off(TextOf(node.Get("note_off"))),
		m_First((int)NumberOf(node.Get("first_note"), 0)),
		m_Toggle(node.Get("toggle_mode") && node.Get("toggle_mode")->AsBool()),
		m_Inverted(false),
		m_ComputerKeys(node.Get("computer_keys") && node.Get("computer_keys")->AsBool()),
		m_Octave(4),
		m_Held(ComputerKeyCount, -1)
	{
		const int keys = (int)NumberOf(node.Get("note_count"), 60);
		const int KeyWidth = (int)NumberOf(node.Get("white_key_width"), 10);
		const Spumoni::JSON *notes = node.Get("notes");
		if (notes && notes->GetType() == Spumoni::JSON::Object)
		{
			const Spumoni::JSON *which = notes->Get("properties");
			for (size_t i = 0; which && which->GetType() == Spumoni::JSON::Array && i < which->Size(); ++i)
				m_Notes.push_back(panel.FindProperty(TextOf(which->At(i))));
			m_Inverted = notes->Get("inverted") && notes->Get("inverted")->AsBool();
		}
		const bool labels = !node.Get("labels") || node.Get("labels")->AsBool();
		int whites = 0;
		for (int n = 0; n < keys; ++n)
			if (!IsBlack(m_First + n))
				++whites;
		m_Group = new PianoKeysGroup(m_X, m_Y, whites * KeyWidth, m_H, *this);
		m_Group->box(FL_FLAT_BOX);
		m_Keys.resize(keys, (Fl_Button *)NULL);

		int Pos = 0, Count = 0;
		for (int n = 0; n < keys; ++n)
		{
			const int note = m_First + n;
			if (IsBlack(note))
				continue;
			Pos = Count * KeyWidth;
			Count++;
			Fl_Button *key = new Fl_Button(m_X + Pos, m_Y, KeyWidth, m_H, "");
			key->box(FL_THIN_UP_BOX);
			key->labelsize(10);
			key->when(FL_WHEN_CHANGED);
			if (labels && note % 12 == 0)
			{
				char label[8];
				std::snprintf(label, sizeof(label), "%d", note / 12);
				key->copy_label(label);
				key->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);
			}
			key->color(FL_WHITE);
			key->selection_color(m_Toggle ? FL_RED : FL_WHITE);
			if (m_Toggle)
				key->type(FL_TOGGLE_BUTTON);
			key->callback(Changed, this);
			m_Keys[n] = key;
		}
		Count = 0;
		for (int n = 0; n < keys; ++n)
		{
			if (!IsBlack(m_First + n))
			{
				Pos = Count * KeyWidth;
				Count++;
				continue;
			}
			Fl_Button *key = new Fl_Button(m_X + Pos + KeyWidth / 2, m_Y, KeyWidth, m_H * 3 / 5, "");
			key->box(FL_THIN_UP_BOX);
			key->labelsize(10);
			key->when(FL_WHEN_CHANGED);
			key->color(FL_BLACK);
			key->selection_color(m_Toggle ? FL_RED : FL_BLACK);
			if (m_Toggle)
				key->type(FL_TOGGLE_BUTTON);
			key->callback(Changed, this);
			m_Keys[n] = key;
		}
		m_Group->end();
		Adopt(m_Group);
	}

	void PanelPianoKeys::Load()
	{
		for (size_t n = 0; n < m_Keys.size() && n < m_Notes.size(); ++n)
		{
			if (!m_Keys[n] || !m_Notes[n])
				continue;
			const bool on = m_Panel.Read(*m_Notes[n]) != 0;
			m_Keys[n]->value(m_Inverted ? !on : on);
		}
	}

	void PanelPianoKeys::Refresh()
	{
		Load();
	}

	void PanelPianoKeys::Changed(Fl_Widget *widget, void *data)
	{
		PanelPianoKeys *self = (PanelPianoKeys *)data;
		for (size_t n = 0; n < self->m_Keys.size(); ++n)
			if (self->m_Keys[n] == widget)
			{
				const bool down = ((Fl_Button *)widget)->value() != 0;
				self->Act(down != self->m_Toggle, self->m_First + (int)n, 0);
				return;
			}
	}

	void PanelPianoKeys::Act(int a, int note, int)
	{
		const int index = note - m_First;
		if (m_On.empty() && index >= 0 && index < (int)m_Notes.size() && m_Notes[index])
		{
			m_Panel.Write(*m_Notes[index], a ? 1 : 0);
			return;
		}
		m_Panel.Command(a ? m_On : m_Off, std::vector<std::string>(1, TextOfNumber(note)));
	}

	bool PanelPianoKeys::Typed(int event)
	{
		if (!m_ComputerKeys)
			return false;

		switch (event)
		{
			case FL_ENTER:
				// Hovering takes the keys without sounding a note, unless a text field is being edited.
				if (!dynamic_cast<Fl_Input_ *>(Fl::focus()))
					Fl::focus(m_Group);
				return true;

			case FL_FOCUS:
				return true;

			case FL_UNFOCUS:
				ReleaseKeys();
				return true;

			case FL_KEYDOWN:
			case FL_KEYUP:
				break;

			default:
				return false;
		}

		const int key = Fl::event_key();
		if (event == FL_KEYDOWN && (Fl::event_state() & (FL_CTRL | FL_ALT | FL_META)))
			return false;

		if (event == FL_KEYDOWN && key >= FL_F + 1 && key <= FL_F + 11)
		{
			m_Octave = key - (FL_F + 1);
			return true;
		}

		const char *found = key > 0 && key < 128 ? std::strchr(ComputerKeys, key) : NULL;
		if (!found)
			return false;

		// The keyboard repeats a held key, which must not play it again.
		const int index = (int)(found - ComputerKeys);
		if (event == FL_KEYDOWN && m_Held[index] < 0)
		{
			m_Held[index] = std::min(m_First + (int)m_Keys.size() - 1, m_Octave * 12 + index);
			Press(m_Held[index], true);
		}
		else if (event == FL_KEYUP && m_Held[index] >= 0)
		{
			Press(m_Held[index], false);
			m_Held[index] = -1;
		}

		return true;
	}

	void PanelPianoKeys::Press(int note, bool down)
	{
		const int index = note - m_First;
		if (index >= 0 && index < (int)m_Keys.size() && m_Keys[index])
		{
			m_Keys[index]->value(down ? 1 : 0);
			m_Keys[index]->redraw();
		}

		Act(down ? 1 : 0, note, 0);
	}

	void PanelPianoKeys::ReleaseKeys()
	{
		for (size_t n = 0; n < m_Held.size(); ++n)
		{
			if (m_Held[n] >= 0)
				Press(m_Held[n], false);

			m_Held[n] = -1;
		}
	}
}
