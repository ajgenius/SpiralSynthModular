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
#include "PanelWindow.h"
#include "libs/libSpiralPanels/include/Panel.h"
#include "DeviceSource.h"
#include "FltkView.h"
#include "PatchDescription.h"
#include "JSON.h"
#include "PanelDocument.h"
#include "PanelFiles.h"
#include "JSON.h"
#include "DeviceClassRegistry.h"
#include "PanelRegistry.h"
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <cstring>
#include "PanelNode.h"
#include "PanelControl.h"
#include <FL/Fl_Window.H>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <cstdlib>

namespace Spiral
{
	namespace Panels
	{
		static const int TitleHeight = 14;
		static const int Margin = 2;

		PanelWindow::PanelWindow(int w, int h, SpiralPlugin *plugin):
			SpiralPluginGUI(w, h, plugin, plugin->GetChannelHandler()),
			m_Layout(NULL),
			m_Mode(NULL),
			m_Config(NULL),
			m_Host(NULL),
			m_Panel(NULL),
			m_Generated(false),
			m_Generation(0)
		{
		}

		// A panel from a file is taken whole or not at all: one that asks for a
		// control this host has not got is not built, so that the device's
		// own panel can be shown in its place.
		FltkPanel *PanelWindow::Build(SpiralPlugin *plugin, const std::string &panel, const std::string &directory,
				const std::string &documentPath, bool &generated, std::string &config, bool whole)
		{
			std::string error, text;
			generated = plugin->GeneratePanel(directory, text);
			Spiral::Panels::Document document;

			if (!document.Load(generated ? text : panel, error))
			{
				std::cerr << "PanelWindow: " << documentPath << ": " << error << std::endl;

				return NULL;
			}

			FltkPanel *host = new FltkPanel(Margin, TitleHeight, new DeviceSource(plugin));
			Panel *model = &host->Model();

			if (!model->Build(document, &error))
			{
				std::cerr << "PanelWindow: " << documentPath << ": " << error << std::endl;
				delete host;

				return NULL;
			}
			for (size_t i = 0; i < model->Unsupported().size(); ++i)
				std::cerr << "PanelWindow: " << documentPath << ": no control registered for \""
					  << model->Unsupported()[i] << "\"" << std::endl;

			if (whole && !generated && !model->Unsupported().empty())
			{
				delete host;

				return NULL;
			}

			for (size_t i = 0; i < model->Unbound().size(); ++i)
				std::cerr << "PanelWindow: " << documentPath << ": unbound " << model->Unbound()[i] << std::endl;

			config = PanelControl::TextOf(document.Panel()->Attribute("header_config"));
			model->Load();

			return host;
		}

		PanelWindow *PanelWindow::Create(SpiralPlugin *plugin, const Spiral::Core::DeviceClass &device)
		{
			if (!plugin)
				return NULL;
			std::string directory = Spiral::Core::DeviceClassRegistry::Get()->Root();
			if (!directory.empty() && directory[directory.size() - 1] != '/')
				directory += '/';
			directory += "panels";
			bool generated = false;
			std::string config;
			FltkPanel *host = NULL;
			std::string document = device.Panel, path = device.Name;

			// A panel file for this device is shown in place of the panel the
			// device has built in; if it cannot be, the built-in one is.
			{
				const Spiral::Core::PanelFiles files(Spiral::Core::PanelFiles::Folders(directory));
				std::string file;
				Spumoni::JSONOwner ui(files.Read("device", device.ID, &file));

				if (ui.get())
				{
					host = Build(plugin, ui->Stringify(false), directory, file, generated, config, true);

					if (host && !generated)
					{
						document = ui->Stringify(false);
						path = file;
					}
					else if (!host)
						std::cerr << "PanelWindow: " << file << ": not used; " << device.Name
							  << (device.Panel.empty() ? " has no panel of its own" : " shows its own panel") << std::endl;
				}
			}

			if (!host)
				host = Build(plugin, device.Panel, directory, device.Name, generated, config);

			if (!host)
				return NULL;

			Panel *panel = &host->Model();
			PanelWindow *window = new PanelWindow(host->w() + 2 * Margin, host->h() + TitleHeight + Margin, plugin);
			window->m_Host = host;
			window->m_Panel = panel;
			window->m_ConfigControl = config;
			window->m_Document = document;
			window->m_Directory = directory;
			window->m_DocumentPath = path;
			window->m_Generated = generated;
			window->m_Generation = plugin->PanelGeneration();
			window->add(host);
			if (panel->LayoutCount() > 1)
			{
				window->m_Layout = new Fl_Button(window->w() - 24, 2, 10, 10, "@-37>");
				window->m_Layout->labeltype(FL_ENGRAVED_LABEL);
				window->m_Layout->labelsize(10);
				window->m_Layout->box(FL_NO_BOX);
				window->m_Layout->down_box(FL_NO_BOX);
				window->m_Layout->callback(NextLayout, window);
				window->m_Layout->copy_tooltip(("Layout: " + panel->LayoutName(0)).c_str());
				window->add(window->m_Layout);
			}
			// After the layout button: the mode button sits beside it.
			window->OfferModes();
			window->LayoutHeader();
			window->end();
			return window;
		}

		SpiralGUIType *PanelWindow::CreateForDevice(SpiralPlugin *plugin, const Spiral::Core::DeviceClass &device)
		{
			return Create(plugin, device);
		}

		void PanelWindow::Provide()
		{
			Spiral::Core::PanelRegistry::Get()->ProvidePanels("fltk", CreateForDevice);
		}

		void PanelWindow::UpdateValues(SpiralPlugin *)
		{
			if (m_Panel)
			{
				m_Panel->Load();
				Fit();
			}
		}

		void PanelWindow::Fit()
		{
			if (m_Host && m_Host->w() + 2 * Margin != w())
				Resize(m_Host->w() + 2 * Margin, h());
			LayoutHeader();
		}

		Fl_Widget *PanelWindow::ConfigWidget() const
		{
			PanelControl *control = m_ConfigControl.empty() ? NULL : m_Panel->FindControl(m_ConfigControl);
			FltkView *view = control ? dynamic_cast<FltkView *>(control->View()) : NULL;
			return view ? view->Widget() : NULL;
		}

		void PanelWindow::OpenConfig(Fl_Widget *button, void *data)
		{
			Fl_Widget *item = static_cast<PanelWindow *>(data)->ConfigWidget();
			if (!item || !item->active()) return;
			if (Fl_Choice *choice = dynamic_cast<Fl_Choice *>(item))
			{
				if (!choice->menu()) return;
				const Fl_Menu_Item *picked = choice->menu()->pulldown(button->x(), button->y(), button->w(), button->h(),
					choice->mvalue(), choice);
				if (picked) choice->picked(picked);
			}
			else if (Fl_Button *action = dynamic_cast<Fl_Button *>(item))
			{
				if (action->type() == FL_TOGGLE_BUTTON) action->value(!action->value());
				action->do_callback();
			}
		}

		void PanelWindow::LayoutHeader()
		{
			Fl_Widget *control = ConfigWidget();
			if (control && (dynamic_cast<Fl_Choice *>(control) || dynamic_cast<Fl_Button *>(control)))
			{
				if (!m_Config)
				{
					m_Config = new Fl_Button(0, 0, 12, 12, "⚙");
					m_Config->box(FL_NO_BOX);
					m_Config->down_box(FL_NO_BOX);
					m_Config->labelsize(12);
					m_Config->callback(OpenConfig, this);
					add(m_Config);
				}
				const char *tip = control->tooltip() ? control->tooltip() : "Device settings";
				if (!m_Config->tooltip() || std::strcmp(m_Config->tooltip(), tip)) m_Config->copy_tooltip(tip);
				if (control->active()) m_Config->activate(); else m_Config->deactivate();
				control->hide();
				m_Config->show();
			}
			else if (m_Config) m_Config->hide();

			Fl_Button *buttons[] = {HelpButton(), m_Config, m_Layout, m_Mode};
			int right = x() + w() - 1;
			for (size_t i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i)
			{
				if (!buttons[i] || !buttons[i]->visible()) continue;
				right -= 12;
				buttons[i]->resize(right, y() + 1, 12, 12);
			}
		}

		void PanelWindow::SaveState(std::map<std::string, Spiral::Core::StateValue> &state) const
		{
			using Spiral::Core::StateValue;
			using Spiral::Data::ControlValue;

			if (!m_Panel)
				return;

			std::map<std::string, ControlValue> values;
			m_Panel->SaveState(values);

			for (std::map<std::string, ControlValue>::const_iterator i = values.begin(); i != values.end(); ++i)
			{
				const ControlValue &value = i->second;

				if (value.type == ControlValue::String)
					state[i->first] = StateValue(StateValue::String, value.text);
				else if (value.type == ControlValue::Boolean)
					state[i->first] = StateValue(StateValue::Boolean, value.boolean ? "true" : "false");
				else
				{
					std::ostringstream text;
					text << value.number;
					state[i->first] = StateValue(StateValue::Number, text.str());
				}
			}

			// Which layout the panel was left in, by its place in the
			// panel's list: the name both trees keep it under.
			if (m_Panel->LayoutCount() > 1)
			{
				std::ostringstream text;
				text << m_Panel->Layout();
				state["LAYOUT"] = StateValue(StateValue::Number, text.str());
			}
		}

		void PanelWindow::LoadState(const std::map<std::string, Spiral::Core::StateValue> &state)
		{
			using Spiral::Core::StateValue;
			using Spiral::Data::ControlValue;

			if (!m_Panel)
				return;

			std::map<std::string, ControlValue> values;

			for (std::map<std::string, StateValue>::const_iterator i = state.begin(); i != state.end(); ++i)
			{
				if (i->second.Type == StateValue::String)
					values[i->first] = ControlValue(i->second.Text);
				else if (i->second.Type == StateValue::Boolean)
					values[i->first] = ControlValue(i->second.Text == "true");
				else
					values[i->first] = ControlValue(std::strtod(i->second.Text.c_str(), NULL));
			}

			m_Panel->LoadState(values);

			std::map<std::string, ControlValue>::const_iterator layout = values.find("LAYOUT");

			if (layout != values.end() && layout->second.number >= 0 && layout->second.number < m_Panel->LayoutCount())
				SetLayout((size_t)layout->second.number);
		}

		SpiralPlugin *PanelWindow::Plugin() const
		{
			DeviceSource *source = m_Panel ? dynamic_cast<DeviceSource *>(m_Panel->Source()) : NULL;

			return source ? source->Plugin() : NULL;
		}

		void PanelWindow::SetLayout(size_t index)
		{
			if (!m_Panel || index >= m_Panel->LayoutCount())
				return;
			m_Panel->ApplyLayout(index);
			Resize(m_Host->w() + 2 * Margin, m_Host->h() + TitleHeight + Margin);
			if (m_Layout)
			{
				m_Layout->copy_tooltip(("Layout: " + m_Panel->LayoutName(index)).c_str());
			}
			LayoutHeader();
			redraw();
		}

		void PanelWindow::SetMode(size_t index)
		{
			if (!m_Panel || index >= m_Panel->ModeCount())
				return;
			m_Panel->SelectMode(index);
			if (m_Mode)
				m_Mode->copy_tooltip(("Panel: " + m_Panel->ModeName(index)).c_str());
		}

		void PanelWindow::NextMode(Fl_Widget *, void *data)
		{
			PanelWindow *self = (PanelWindow *)data;
			if (self->m_Panel && self->m_Panel->ModeCount())
				self->SetMode((self->m_Panel->Mode() + 1) % self->m_Panel->ModeCount());
		}

		void PanelWindow::NextLayout(Fl_Widget *, void *data)
		{
			PanelWindow *self = (PanelWindow *)data;
			if (self->m_Panel && self->m_Panel->LayoutCount())
				self->SetLayout((self->m_Panel->Layout() + 1) % self->m_Panel->LayoutCount());
		}

		static bool OnScreen(const Fl_Widget *w)
		{
			if (!w->visible_r())
				return false;
			int X = w->x(), Y = w->y(), R = X + w->w(), B = Y + w->h();
			for (const Fl_Widget *p = w->parent(); p; p = p->parent())
			{
				const bool window = p->type() >= FL_WINDOW;
				X = std::max(X, window ? 0 : p->x());
				Y = std::max(Y, window ? 0 : p->y());
				R = std::min(R, window ? p->w() : p->x() + p->w());
				B = std::min(B, window ? p->h() : p->y() + p->h());
				if (window)
					break;
			}
			return X < R && Y < B;
		}

		void PanelWindow::Update()
		{
			if (m_Generated && Plugin() && Plugin()->PanelGeneration() != m_Generation)
				Rebuild();
			if (m_Panel && OnScreen(this))
			{
				m_Panel->Refresh();
				Fit();
			}
		}

		void PanelWindow::Rebuild()
		{
			SpiralPlugin *plugin = Plugin();
			m_Generation = plugin->PanelGeneration();
			bool generated = false;
			std::string config;
			FltkPanel *host = Build(plugin, m_Document, m_Directory, m_DocumentPath, generated, config);

			if (!host)
				return;

			remove(m_Host);
			delete m_Host;
			m_Host = host;
			m_Panel = &host->Model();
			m_ConfigControl = config;
			host->position(x() + Margin, y() + TitleHeight);
			add(host);
			Resize(host->w() + 2 * Margin, host->h() + TitleHeight + Margin);
			OfferModes();
			LayoutHeader();
			redraw();
		}

		void PanelWindow::OfferModes()
		{
			if (!m_Panel || m_Panel->ModeCount() < 2)
			{
				if (m_Mode)
					m_Mode->hide();

				return;
			}

			if (!m_Mode)
			{
				m_Mode = new Fl_Button(0, 0, 10, 10, "@square");
				m_Mode->labeltype(FL_ENGRAVED_LABEL);
				m_Mode->labelsize(10);
				m_Mode->box(FL_NO_BOX);
				m_Mode->down_box(FL_NO_BOX);
				m_Mode->callback(NextMode, this);
				add(m_Mode);
			}

			m_Mode->copy_tooltip(("Panel: " + m_Panel->ModeName(m_Panel->Mode())).c_str());
			m_Mode->show();
		}

		const std::string PanelWindow::GetHelpText(const std::string &loc)
		{
			if (m_Panel && !m_Panel->Help().empty())
				return m_Panel->Help();
			return SpiralPluginGUI::GetHelpText(loc);
		}
	}
}
