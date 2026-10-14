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
#include "PanelListChoosingButton.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Hold_Browser.H>

namespace SpiralPanels
{

	PanelControl *PanelListChoosingButton::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelListChoosingButton(panel, node);
	}

	PanelListChoosingButton::PanelListChoosingButton(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node)
	{
		const Spumoni::JSON *list = node.Get("list");
		if (list)
		{
			const Spumoni::JSON *options = list->Get("options");
			for (size_t i = 0; options && i < options->Size(); ++i)
				m_Options.push_back(TextOf(options->At(i)));
		}
		m_Button = new Fl_Button(m_X, m_Y, m_W, m_H);
		m_Button->copy_label(TextOf(node.Get("text")).c_str());
		m_Button->labelsize((int)NumberOf(node.Get("size"), 10));
		m_Button->box(FL_PLASTIC_UP_BOX);
		m_Button->color((Fl_Color)GUIColour());
		m_Button->selection_color((Fl_Color)GUIColour());
		m_Button->callback(Clicked, this);
		Adopt(m_Button);
	}

	int PanelListChoosingButton::ChooseFrom(const std::vector<std::string> &options)
	{
		Fl_Double_Window *window = new Fl_Double_Window(300, 300);
		Fl_Button *ok = new Fl_Button(10, 275, 40, 20, "Ok");
		ok->labelsize(10);
		Fl_Button *cancel = new Fl_Button(50, 275, 40, 20, "Cancel");
		cancel->labelsize(10);
		Fl_Hold_Browser *browser = new Fl_Hold_Browser(5, 5, 290, 265, "");
		for (size_t i = 0; i < options.size(); ++i)
			browser->add(options[i].c_str());
		window->end();
		window->set_modal();
		window->show();
		int choice = -1;
		for (;;)
		{
			Fl::wait();
			Fl_Widget *o = Fl::readqueue();
			if (o == ok || o == browser)
			{
				choice = browser->value() - 1;
				break;
			}
			if (o == cancel || o == window || !window->shown())
				break;
		}
		window->hide();
		delete window;
		return choice;
	}

	void PanelListChoosingButton::Choose()
	{
		Finish(ChooseFrom(m_Options));
	}

	void PanelListChoosingButton::Finish(int choice)
	{
		if (choice < 0 || choice >= (int)m_Options.size())
			return;
		Fire("value_changed", choice, m_Options[choice]);
	}

	void PanelListChoosingButton::Clicked(Fl_Widget *, void *data)
	{
		((PanelListChoosingButton *)data)->Choose();
	}
}
