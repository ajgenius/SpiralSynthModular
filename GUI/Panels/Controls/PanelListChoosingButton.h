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
#ifndef SPIRAL_PANELS_PANELLISTCHOOSINGBUTTON_H
#define SPIRAL_PANELS_PANELLISTCHOOSINGBUTTON_H

#include "PanelControl.h"
#include <string>
#include <vector>

class Fl_Button;
class Fl_Widget;

namespace SpiralPanels
{
	class PanelListChoosingButton: public PanelControl
	{
	public:
		static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

		void Finish(int choice);
		size_t OptionCount() const { return m_Options.size(); }
		static int ChooseFrom(const std::vector<std::string> &options);
		// A press opens a list to pick from: not something a controller should do.
		virtual bool Drivable() const { return false; }

	protected:
		PanelListChoosingButton(Panel &panel, const Spumoni::JSON &node);

		void Choose();
		static void Clicked(Fl_Widget *widget, void *data);

		std::vector<std::string> m_Options;
		Fl_Button *m_Button;
	};
}

#endif
