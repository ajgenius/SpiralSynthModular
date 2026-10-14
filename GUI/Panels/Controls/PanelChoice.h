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
#ifndef SPIRAL_PANELS_PANELCHOICE_H
#define SPIRAL_PANELS_PANELCHOICE_H

#include "PanelControl.h"
#include <vector>

class Fl_Choice;
class Fl_Widget;

namespace SpiralPanels
{
	class PanelChoice: public PanelControl
	{
	public:
		static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

		virtual void Load();

	protected:
		PanelChoice(Panel &panel, const Spumoni::JSON &node);

		static void Changed(Fl_Widget *widget, void *data);

		Fl_Choice *m_Choice;
		std::vector<double> m_Values;
		std::vector<int> m_Indices;
	};
}

#endif
