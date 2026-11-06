// SPDX-License-Identifier: GPL-2.0-or-later
#include "Patch.h"
#include "FltkView.h"
#include "FltkTheme.h"
#include "JSON.h"
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>
#include <cstdio>
#include <vector>

namespace Spiral
{
	namespace Panels
	{
		namespace
		{
			class FltkPorts: public FltkView, public virtual PortsView
			{
			public:
				FltkPorts(Fl_Scroll *scroll, Fl_Pack *outputs, Fl_Pack *inputs, PortListControl &control,
						const FltkLook &label, const FltkLook &button):
					FltkView(scroll), m_Scroll(scroll), m_Control(control), m_LabelLook(label), m_ButtonLook(button)
				{
					m_Packs[0] = outputs;
					m_Packs[1] = inputs;
				}

				void SetPortCount(int column, int count, const std::string &labelFormat)
				{
					std::vector<Fl_Box *> &labels = m_Labels[column];
					std::vector<Fl_Button *> &buttons = m_Buttons[column];
					Fl_Pack *pack = m_Packs[column];
					const int x = column == 0 ? 0 : 95;

					while ((int)buttons.size() > count && !buttons.empty())
					{
						pack->remove(labels.back());
						pack->remove(buttons.back());
						delete labels.back();
						delete buttons.back();
						labels.pop_back();
						buttons.pop_back();
					}

					while ((int)buttons.size() < count)
					{
						const int n = (int)buttons.size();
						char text[64];
						std::snprintf(text, sizeof(text), labelFormat.c_str(), n);
						Fl_Box *label = new Fl_Box(x, n * 30, 90, 10);
						label->copy_label(text);
						m_LabelLook.Apply(label);
						pack->add(label);
						Fl_Button *button = new Fl_Button(x, n * 30 + 10, 90, 20, "None");
						button->type(FL_TOGGLE_BUTTON);
						m_ButtonLook.Apply(button);
						button->callback(Clicked, this);
						pack->add(button);
						labels.push_back(label);
						buttons.push_back(button);
					}

					pack->redraw();
					m_Scroll->redraw();
				}

				int PortCount(int column) const
				{
					return (int)m_Buttons[column].size();
				}

				void SetPort(int column, int index, const std::string &name, bool connected)
				{
					if (index < 0 || index >= (int)m_Buttons[column].size())
						return;

					Fl_Button *button = m_Buttons[column][index];
					button->value(connected);
					button->copy_label(name.empty() ? "None" : name.c_str());
					button->redraw();
					m_Packs[column]->redraw();
				}

			private:
				static void Clicked(Fl_Widget *widget, void *data)
				{
					FltkPorts *self = static_cast<FltkPorts *>(data);

					for (int column = 0; column < 2; ++column)
					{
						for (size_t n = 0; n < self->m_Buttons[column].size(); ++n)
						{
							if (self->m_Buttons[column][n] == widget)
							{
								self->m_Control.Clicked(column, (int)n, static_cast<Fl_Button *>(widget)->value() != 0);

								return;
							}
						}
					}
				}

				Fl_Scroll *m_Scroll;
				Fl_Pack *m_Packs[2];
				std::vector<Fl_Box *> m_Labels[2];
				std::vector<Fl_Button *> m_Buttons[2];
				PortListControl &m_Control;
				FltkLook m_LabelLook;
				FltkLook m_ButtonLook;
			};

			ControlView *MakePortList(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_Scroll *scroll = new Fl_Scroll(control.X(), control.Y(), control.W(), control.H());
				FltkTheme::For(control, "PortList", node).Apply(scroll);
				scroll->type(Fl_Scroll::VERTICAL_ALWAYS);
				Fl_Pack *outputs = new Fl_Pack(control.X() + 10, control.Y(), 85, control.H());
				outputs->end();
				Fl_Pack *inputs = new Fl_Pack(control.X() + 105, control.Y(), 85, control.H());
				inputs->end();
				scroll->end();

				return new FltkPorts(scroll, outputs, inputs, static_cast<PortListControl &>(control),
					FltkTheme::For(control, "PortListLabel", node), FltkTheme::For(control, "PortListButton", node));
			}

			ViewRegistry::Entry s_PortList("PortList", &MakePortList);
		}
	}
}
