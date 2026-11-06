// SPDX-License-Identifier: GPL-2.0-or-later
#include "Patch.h"
#include "ControlSupport.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_PianoKeys("PianoKeys", &PianoKeysControl::Create);

		static const char KeyMapText[] = "zsxdcvgbhnjmq2w3er5t6y7ui9o0p[";

		bool PianoKeysControl::IsBlack(int note)
		{
			const int n = note % 12;

			return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
		}

		const char *PianoKeysControl::KeyMap()
		{
			return KeyMapText;
		}

		int PianoKeysControl::KeyMapSize()
		{
			return (int)sizeof(KeyMapText) - 1;
		}

		PanelControl *PianoKeysControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new PianoKeysControl(panel, node));
		}

		PianoKeysControl::PianoKeysControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Keys(NULL),
			m_On(TextOf(node.Get("note_on"))),
			m_Off(TextOf(node.Get("note_off"))),
			m_First((int)NumberOf(node.Get("first_note"), 0)),
			m_Count((int)NumberOf(node.Get("note_count"), 60)),
			m_Toggle(node.Get("toggle_mode") && node.Get("toggle_mode")->AsBool()),
			m_Inverted(false),
			m_ComputerKeys(node.Get("computer_keys") && node.Get("computer_keys")->AsBool()),
			m_Octave(4),
			m_Held(KeyMapSize(), -1),
			m_Status(TextOf(node.Get("status"))),
			m_Framed(false),
			m_FramedOctave(-1)
		{
			const Spumoni::JSON *notes = node.Get("notes");

			if (notes && notes->GetType() == Spumoni::JSON::Object)
			{
				const Spumoni::JSON *which = notes->Get("properties");

				for (size_t i = 0; which && which->GetType() == Spumoni::JSON::Array && i < which->Size(); ++i)
					m_Notes.push_back(panel.FindProperty(TextOf(which->At(i))));

				m_Inverted = notes->Get("inverted") && notes->Get("inverted")->AsBool();
			}

			m_Keys = MakeView<KeysView>("PianoKeys", *this, node);

			if (m_Keys)
				Adopt(m_Keys);
		}

		void PianoKeysControl::Load()
		{
			for (int n = 0; n < m_Count && n < (int)m_Notes.size(); ++n)
			{
				if (!m_Notes[n])
					continue;

				const bool on = m_Panel.Read(*m_Notes[n]) != 0;
				m_Keys->SetKey(n, m_Inverted ? !on : on);
			}
		}

		void PianoKeysControl::Refresh()
		{
			Load();
			RefreshFocus();
		}

		int PianoKeysControl::LastMapped() const
		{
			return std::min(m_First + m_Count - 1, m_Octave * 12 + KeyMapSize() - 1);
		}

		void PianoKeysControl::RefreshFocus()
		{
			const bool focused = m_ComputerKeys && m_Keys->Focused();
			const int last = LastMapped();

			if (focused != m_Framed || m_Octave != m_FramedOctave)
			{
				m_Keys->Frame(focused, m_Octave * 12, last);
				m_Framed = focused;
				m_FramedOctave = m_Octave;
			}

			if (m_Status.empty())
				return;

			char text[64];

			if (focused)
				std::snprintf(text, sizeof(text), "Octaves %d-%d (F%d) | F1-F11", m_Octave, last / 12, m_Octave + 1);
			else
				std::snprintf(text, sizeof(text), "Hover panel to play | F1-F11: octave");

			if (m_StatusText == text)
				return;

			LabelControl *status = dynamic_cast<LabelControl *>(m_Panel.FindControl(m_Status));

			if (status)
			{
				status->Show(text);
				m_StatusText = text;
			}
		}

		void PianoKeysControl::KeyChanged(int index, bool down)
		{
			Act(down != m_Toggle, m_First + index);
		}

		void PianoKeysControl::Act(bool on, int note)
		{
			const int index = note - m_First;

			if (m_On.empty() && index >= 0 && index < (int)m_Notes.size() && m_Notes[index])
			{
				m_Panel.Write(*m_Notes[index], on ? 1 : 0);

				return;
			}

			m_Panel.Command(on ? m_On : m_Off, std::vector<std::string>(1, TextOfNumber(note)));
		}

		void PianoKeysControl::Entered()
		{
			RefreshFocus();
		}

		void PianoKeysControl::Unfocused()
		{
			ReleaseKeys();
			RefreshFocus();
		}

		void PianoKeysControl::SelectOctave(int octave)
		{
			m_Octave = octave;
			m_Keys->ShowNote(m_Octave * 12);
			RefreshFocus();
		}

		bool PianoKeysControl::Typed(char key, bool down)
		{
			const char *found = key > 0 ? std::strchr(KeyMapText, key) : NULL;

			if (!found)
				return false;

			const int index = (int)(found - KeyMapText);

			if (down && m_Held[index] < 0)
			{
				m_Held[index] = std::min(m_First + m_Count - 1, m_Octave * 12 + index);
				Press(m_Held[index], true);
			}
			else if (!down && m_Held[index] >= 0)
			{
				Press(m_Held[index], false);
				m_Held[index] = -1;
			}

			return true;
		}

		void PianoKeysControl::Press(int note, bool down)
		{
			const int index = note - m_First;

			if (index >= 0 && index < m_Count)
				m_Keys->SetKey(index, down);

			Act(down, note);
		}

		void PianoKeysControl::ReleaseKeys()
		{
			for (size_t n = 0; n < m_Held.size(); ++n)
			{
				if (m_Held[n] >= 0)
					Press(m_Held[n], false);

				m_Held[n] = -1;
			}
		}
	}
}
