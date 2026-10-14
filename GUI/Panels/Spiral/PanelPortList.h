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
#ifndef SPIRAL_PANELS_PANELPORTLIST_H
#define SPIRAL_PANELS_PANELPORTLIST_H

#include "PanelControl.h"
#include <string>
#include <vector>

class Fl_Box;
class Fl_Button;
class Fl_Pack;
class Fl_Scroll;
class Fl_Widget;

namespace SpiralPanels
{
	class PanelPortList: public PanelControl
	{
	public:
		static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

		virtual void Load();
		virtual void Refresh();

		size_t OutputCount() const { return m_Outputs.Buttons.size(); }
		size_t InputCount() const { return m_Inputs.Buttons.size(); }

	protected:
		PanelPortList(Panel &panel, const Spumoni::JSON &node);

		struct Column
		{
			Column(): Count(NULL), NamesCount(NULL), Pack(NULL) {}
			const PanelProperty *Count, *NamesCount;
			std::string Label, Connections, Names;
			std::string Connect, Disconnect;
			Fl_Pack *Pack;
			std::vector<unsigned char> Shown;
			std::vector<Fl_Box *> Labels;
			std::vector<Fl_Button *> Buttons;
		};
		void Read(Column &column, const Spumoni::JSON *node);
		void Sync(Column &column, int x);
		void Show(Column &column);
		void Clicked(Column &column, Fl_Button *button);
		static void OutputClicked(Fl_Widget *widget, void *data);
		static void InputClicked(Fl_Widget *widget, void *data);

		Fl_Scroll *m_Scroll;
		Column m_Outputs, m_Inputs;
		const PanelProperty *m_Connected;
		std::string m_NamesCommand, m_RefreshCommand;
	};
}

#endif
