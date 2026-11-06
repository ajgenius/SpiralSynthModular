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
#include "PanelPortList.h"
#include "PanelListChoosingButton.h"
#include "Panel.h"
#include "DeviceSource.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>
#include <cstdio>
#include <cstring>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("PortList", &PanelPortList::Create);

	static const int NAME_SIZE = 256;

	PanelControl *PanelPortList::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelPortList(panel, node);
	}

	PanelPortList::PanelPortList(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_Connected(panel.FindProperty(TextOf(node.Get("connected")))),
		m_NamesCommand(TextOf(node.Get("list"))),
		m_RefreshCommand(TextOf(node.Get("refresh")))
	{
		Read(m_Outputs, node.Get("outputs"));
		Read(m_Inputs, node.Get("inputs"));
		m_Scroll = new Fl_Scroll(m_X, m_Y, m_W, m_H);
		m_Scroll->box(FL_PLASTIC_DOWN_BOX);
		m_Scroll->type(Fl_Scroll::VERTICAL_ALWAYS);
		m_Outputs.Pack = new Fl_Pack(m_X + 10, m_Y, 85, m_H);
		m_Outputs.Pack->end();
		m_Inputs.Pack = new Fl_Pack(m_X + 105, m_Y, 85, m_H);
		m_Inputs.Pack->end();
		m_Scroll->end();
		Adopt(m_Scroll);
	}

	void PanelPortList::Read(Column &column, const Spumoni::JSON *node)
	{
		if (!node)
			return;
		column.Count = m_Panel.FindProperty(TextOf(node->Get("count")));
		column.NamesCount = m_Panel.FindProperty(TextOf(node->Get("names_count")));
		column.Label = TextOf(node->Get("label"));
		column.Connections = TextOf(node->Get("connections"));
		column.Names = TextOf(node->Get("names"));
		column.Connect = TextOf(node->Get("connect"));
		column.Disconnect = TextOf(node->Get("disconnect"));
	}

	void PanelPortList::Load()
	{
		Sync(m_Outputs, 0);
		Sync(m_Inputs, 95);
		if (!m_RefreshCommand.empty())
			m_Panel.Command(m_RefreshCommand);
		Show(m_Outputs);
		Show(m_Inputs);
	}

	void PanelPortList::Sync(Column &column, int x)
	{
		if (!column.Count || !column.Pack)
			return;
		const int count = (int)m_Panel.Read(*column.Count);
		while ((int)column.Buttons.size() > count && !column.Buttons.empty())
		{
			column.Pack->remove(column.Labels.back());
			column.Pack->remove(column.Buttons.back());
			delete column.Labels.back();
			delete column.Buttons.back();
			column.Labels.pop_back();
			column.Buttons.pop_back();
		}
		while ((int)column.Buttons.size() < count)
		{
			const int n = (int)column.Buttons.size();
			char text[64];
			std::snprintf(text, sizeof(text), column.Label.c_str(), n);
			Fl_Box *label = new Fl_Box(x, n * 30, 90, 10);
			label->copy_label(text);
			label->labelsize(8);
			column.Pack->add(label);
			Fl_Button *button = new Fl_Button(x, n * 30 + 10, 90, 20, "None");
			button->type(FL_TOGGLE_BUTTON);
			button->labelsize(8);
			button->callback(&column == &m_Outputs ? OutputClicked : InputClicked, this);
			column.Pack->add(button);
			column.Labels.push_back(label);
			column.Buttons.push_back(button);
			column.Shown.clear();
		}
		column.Pack->redraw();
		m_Scroll->redraw();
	}

	void PanelPortList::Refresh()
	{
		Show(m_Outputs);
		Show(m_Inputs);
	}

	void PanelPortList::Show(Column &column)
	{
		if (!m_Panel.Source())
			return;
		std::vector<unsigned char> bytes;
		m_Panel.Source()->ReadBytes(column.Connections, bytes);
		if (bytes == column.Shown && !column.Shown.empty())
			return;
		column.Shown = bytes;
		for (size_t n = 0; n < column.Buttons.size(); ++n)
		{
			std::string name;
			if ((n + 1) * NAME_SIZE <= bytes.size())
				name = std::string((const char *)&bytes[n * NAME_SIZE], strnlen((const char *)&bytes[n * NAME_SIZE], NAME_SIZE));
			column.Buttons[n]->value(!name.empty());
			column.Buttons[n]->copy_label(name.empty() ? "None" : name.c_str());
		}
		column.Pack->redraw();
	}

	void PanelPortList::Clicked(Column &column, Fl_Button *button)
	{
		int index = 0;
		for (size_t n = 0; n < column.Buttons.size(); ++n)
			if (column.Buttons[n] == button)
				index = (int)n;
		DeviceSource *binding = m_Panel.Source();
		const bool attached = m_Connected && m_Panel.Read(*m_Connected) != 0;
		if (button->value() && attached && binding)
		{
			m_Panel.Command(m_NamesCommand);
			binding->Wait();
			std::vector<unsigned char> bytes;
			binding->ReadBytes(column.Names, bytes);
			double counted = 0;
			if (column.NamesCount)
				binding->ReadNow(*column.NamesCount, counted);
			const int count = (int)counted;
			std::vector<std::string> names;
			for (int n = 0; n < count && (size_t)(n + 1) * NAME_SIZE <= bytes.size(); ++n)
				names.push_back(std::string((const char *)&bytes[n * NAME_SIZE], strnlen((const char *)&bytes[n * NAME_SIZE], NAME_SIZE)));
			const int choice = PanelListChoosingButton::ChooseFrom(names);
			if (choice >= 0)
			{
				std::vector<std::string> args;
				args.push_back(TextOfNumber(index));
				args.push_back(names[choice]);
				m_Panel.Command(column.Connect, args);
				binding->Wait();
				button->copy_label(names[choice].c_str());
				return;
			}
		}
		m_Panel.Command(column.Disconnect, std::vector<std::string>(1, TextOfNumber(index)));
		if (binding && attached)
			binding->Wait();
		button->label("None");
		button->value(0);
		button->redraw();
	}

	void PanelPortList::OutputClicked(Fl_Widget *widget, void *data)
	{
		PanelPortList *self = (PanelPortList *)data;
		self->Clicked(self->m_Outputs, (Fl_Button *)widget);
	}

	void PanelPortList::InputClicked(Fl_Widget *widget, void *data)
	{
		PanelPortList *self = (PanelPortList *)data;
		self->Clicked(self->m_Inputs, (Fl_Button *)widget);
	}
}
