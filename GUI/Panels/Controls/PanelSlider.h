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
#ifndef SPIRAL_PANELS_PANELSLIDER_H
#define SPIRAL_PANELS_PANELSLIDER_H

#include "PanelControl.h"
#include <string>

class Fl_Slider;
class Fl_Widget;

namespace SpiralPanels
{
	class PanelSlider: public PanelControl
	{
	public:
		static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

		virtual void Load();
		virtual void Refresh();

	protected:
		PanelSlider(Panel &panel, const Spumoni::JSON &node);

		static void Changed(Fl_Widget *widget, void *data);
		void ShowValue(double value);

		Fl_Slider *m_Slider;
		std::string m_ValueLabel;
		double m_StepFraction;
		bool m_Flip;
	};
}

#endif
