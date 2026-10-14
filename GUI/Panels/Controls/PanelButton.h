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
#ifndef SPIRAL_PANELS_PANELBUTTON_H
#define SPIRAL_PANELS_PANELBUTTON_H

#include "PanelControl.h"
#include <utility>
#include <string>
#include <vector>

class Fl_Button;
class Fl_Widget;

namespace SpiralPanels
{
	class PanelProperty;
}

namespace SpiralPanels
{
	class PanelButton: public PanelControl
	{
	public:
		static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

		virtual void Load();
		virtual void Tick();

	protected:
		PanelButton(Panel &panel, const Spumoni::JSON &node);

		static void Changed(Fl_Widget *widget, void *data);
		void DeclareEvents(std::vector<Event> &out) const;
		// The label, when it is bound: `text` as a binding, or the older `text_when`.
		Spiral::Data::ValueRule m_TextRule;
		bool m_TextBound;
		std::string m_Text;

		Fl_Button *m_Button;
		bool m_HasChoice;
		double m_Choice;
	};
}

#endif
