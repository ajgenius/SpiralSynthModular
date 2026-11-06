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
#ifndef SPIRAL_PANELS_PANELPIANOKEYS_H
#define SPIRAL_PANELS_PANELPIANOKEYS_H

#include "PanelControl.h"
#include <string>
#include <vector>

class Fl_Button;
class Fl_Group;
class Fl_Widget;

namespace SpiralPanels
{
	class PanelPianoKeys: public PanelControl
	{
	public:
		static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

		virtual void Load();
		virtual void Refresh();
		void Act(int a, int note, int delta);
		bool Typed(int event);
		void DrawOverlay();

	protected:
		PanelPianoKeys(Panel &panel, const Spumoni::JSON &node);

		static void Changed(Fl_Widget *widget, void *data);
		void Press(int note, bool down);
		void ReleaseKeys();
		Fl_Button *Key(int note) const;
		bool Focused() const;
		int LastMapped() const;
		void RefreshFocus();
		void ShowOctave();

		std::string m_On, m_Off;
		int m_First;
		bool m_Toggle;
		std::vector<const PanelProperty *> m_Notes;
		bool m_Inverted;
		Fl_Group *m_Group;
		std::vector<Fl_Button *> m_Keys;
		bool m_ComputerKeys;
		int m_Octave;
		std::vector<int> m_Held;
		std::string m_Status;
		std::string m_StatusText;
		bool m_Framed;
		int m_FramedOctave;
	};
}

#endif
