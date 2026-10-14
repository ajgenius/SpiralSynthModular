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
#ifndef SPIRAL_PANELS_PANELFUNCTIONPLOT_H
#define SPIRAL_PANELS_PANELFUNCTIONPLOT_H

#include "PanelControl.h"
#include <string>
#include <vector>

class Fl_Widget;

namespace SpiralPanels
{
	class PanelFunctionPlot: public PanelControl
	{
	public:
		static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

		virtual void Load();
		virtual void Refresh();

	protected:
		PanelFunctionPlot(Panel &panel, const Spumoni::JSON &node);

		std::string m_Channel;
		std::vector<float> m_Values;
		Fl_Widget *m_View;
	};
}

#endif
