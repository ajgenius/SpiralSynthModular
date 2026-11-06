// SPDX-License-Identifier: GPL-2.0-or-later
#include "Controls.h"
#include "FltkSupport.h"
#include "FltkTheme.h"
#include "Fl_Knob.H"
#include "Fl_LED_Button.H"
#include "Fl_VirtualScrollArea.H"
#include "JSON.h"
#include "PanelProperty.h"
#include "WS_InvSine.h"
#include "WS_Noise.h"
#include "WS_Pulse1.h"
#include "WS_Pulse2.h"
#include "WS_RevSaw.h"
#include "WS_Saw.h"
#include "WS_Sine.h"
#include "WS_Square.h"
#include "WS_Tri.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Repeat_Button.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Valuator.H>

namespace Spiral
{
	namespace Panels
	{
		namespace
		{
			class FltkValue: public FltkView, public virtual ValueView
			{
			public:
				explicit FltkValue(Fl_Valuator *valuator): FltkView(valuator), m_Valuator(valuator) {}

				void SetRange(double low, double high) { m_Valuator->range(low, high); }
				double Low() const { return m_Valuator->minimum(); }
				double High() const { return m_Valuator->maximum(); }
				void SetStep(double step) { m_Valuator->step(step); }
				void SetValue(double value) { m_Valuator->value(value); }

			protected:
				Fl_Valuator *m_Valuator;
			};

			class FltkSlider: public FltkValue, public virtual SliderView
			{
			public:
				explicit FltkSlider(Fl_Slider *slider): FltkValue(slider) {}

				void SetValueLabel(const std::string &text, bool left)
				{
					m_Valuator->align(FL_ALIGN_INSIDE | (left ? FL_ALIGN_LEFT : FL_ALIGN_RIGHT));
					m_Valuator->copy_label(text.c_str());
				}
			};

			class FltkCheck: public FltkView, public virtual CheckView
			{
			public:
				FltkCheck(Fl_Button *button, Fl_Pixmap *glyph = NULL): FltkView(button), m_Button(button), m_Glyph(glyph) {}
				~FltkCheck() { delete m_Glyph; }

				void SetChecked(bool checked) { m_Button->value(checked); }

			private:
				Fl_Button *m_Button;
				Fl_Pixmap *m_Glyph;
			};

			class FltkSpinner: public FltkValue, public virtual SpinnerView
			{
			public:
				explicit FltkSpinner(Fl_Counter *counter): FltkValue(counter), m_Counter(counter) {}

				void SetSteps(double step, double page) { m_Counter->step(step, page); }

			private:
				Fl_Counter *m_Counter;
			};

			class FltkText: public FltkView, public virtual TextView
			{
			public:
				explicit FltkText(Fl_Input *input): FltkView(input), m_Input(input) {}

				void SetText(const std::string &text) { m_Input->value(text.c_str()); }

			private:
				Fl_Input *m_Input;
			};

			class FltkChoice: public FltkView, public virtual ChoiceView
			{
			public:
				explicit FltkChoice(Fl_Choice *choice): FltkView(choice), m_Choice(choice) {}

				int AddItem(const std::string &text, bool literal)
				{
					if (!literal)
						return m_Choice->add(text.c_str());

					const int index = m_Choice->add("x");
					m_Choice->replace(index, text.c_str());

					return index;
				}

				void Select(int index) { m_Choice->value(index); }

			private:
				Fl_Choice *m_Choice;
			};

			class FltkTabs: public FltkView, public virtual ContainerView
			{
			public:
				FltkTabs(Fl_Tabs *tabs, const FltkLook &pageLook):
					FltkView(tabs),
					m_Tabs(tabs),
					m_Page(NULL),
					m_Pages(0),
					m_PageLook(pageLook)
				{
				}

				void BeginPage(const std::string &title, int x, int y, int w, int h)
				{
					m_Page = new Fl_Group(x, y, w, h);
					m_Page->copy_label(title.c_str());
					m_PageLook.Apply(m_Page);
				}

				void EndPage()
				{
					m_Page->end();

					if (m_Pages++ > 0)
						m_Page->hide();
				}

				void End(int, int, bool, int, int) { m_Tabs->end(); }

			private:
				Fl_Tabs *m_Tabs;
				Fl_Group *m_Page;
				int m_Pages;
				FltkLook m_PageLook;
			};

			class FltkScrollArea: public FltkView, public virtual ContainerView
			{
			public:
				explicit FltkScrollArea(Fl_VirtualScrollArea *area): FltkView(area), m_Area(area) {}

				void BeginPage(const std::string &, int, int, int, int) {}
				void EndPage() {}

				void End(int contentW, int contentH, bool hasView, int viewX, int viewY)
				{
					m_Area->end();
					m_Area->SetContentSize(contentW, contentH);

					if (hasView)
						m_Area->SetView(viewX, viewY);
				}

			private:
				Fl_VirtualScrollArea *m_Area;
			};

			class FltkDialogs: public Dialogs
			{
			public:
				FltkDialogs() { Dialogs::Set(this); }

				int ChooseFrom(const std::vector<std::string> &options)
				{
					Fl_Double_Window *window = new Fl_Double_Window(300, 300);
					const FltkLook look = FltkTheme::Base("Dialog");
					Fl_Button *ok = new Fl_Button(10, 275, 40, 20, "Ok");
					look.Apply(ok);
					Fl_Button *cancel = new Fl_Button(50, 275, 40, 20, "Cancel");
					look.Apply(cancel);
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

				void ChooseFile(bool, const std::string &title, const std::string &filter, Completed done, void *request)
				{
					done(request, fl_file_chooser(title.c_str(), filter.empty() ? NULL : filter.c_str(), NULL));
				}
			};

			FltkDialogs s_Dialogs;

			class Counter: public Fl_Counter
			{
			public:
				Counter(int x, int y, int w, int h): Fl_Counter(x, y, w, h) {}

				int handle(int event)
				{
					if (event == FL_PUSH)
						Fl::focus(this);

					return Fl_Counter::handle(event);
				}
			};

			class FltkLabel: public FltkView, public virtual LabelView
			{
			public:
				explicit FltkLabel(Fl_Box *box): FltkView(box), m_Box(box) {}

				void SetText(const std::string &text) { m_Box->copy_label(text.c_str()); }

			private:
				Fl_Box *m_Box;
			};

			void KnobChanged(Fl_Widget *widget, void *data)
			{
				static_cast<KnobControl *>(data)->Changed(static_cast<Fl_Valuator *>(widget)->value());
			}

			ControlView *MakeKnob(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_Knob *knob = new Fl_Knob(control.X(), control.Y(), control.W(), control.H());
				const FltkLook look = FltkTheme::For(control, "Knob", node);
				look.Apply(knob);
				knob->type(look.Text("knob_type") == "dot" ? Fl_Knob::DOTLIN : Fl_Knob::LINELIN);
				knob->scaleticks((int)Number(node, "scale_ticks", 10));
				knob->callback(KnobChanged, static_cast<KnobControl *>(&control));

				return new FltkValue(knob);
			}

			void SliderChanged(Fl_Widget *widget, void *data)
			{
				static_cast<SliderControl *>(data)->Changed(static_cast<Fl_Valuator *>(widget)->value());
			}

			ControlView *MakeSlider(PanelControl &control, const Spumoni::JSON &node)
			{
				SliderControl &slider = static_cast<SliderControl &>(control);
				Fl_Slider *widget = new Fl_Slider(control.X(), control.Y(), control.W(), control.H());
				const FltkLook look = FltkTheme::For(control, "Slider", node, slider.HasValueLabel() ? "value_label" : "");

				if (look.Text("slider_type") == "plain")
					widget->type(slider.Vertical() ? FL_VERTICAL : FL_HORIZONTAL);
				else
					widget->type(slider.Vertical() ? FL_VERT_NICE_SLIDER : FL_HOR_NICE_SLIDER);

				look.Apply(widget);

				widget->callback(SliderChanged, &slider);

				return new FltkSlider(widget);
			}

			void CheckChanged(Fl_Widget *widget, void *data)
			{
				static_cast<CheckBoxControl *>(data)->Changed(static_cast<Fl_Button *>(widget)->value() != 0);
			}

			ControlView *MakeCheckBox(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_Check_Button *button = new Fl_Check_Button(control.X(), control.Y(), control.W(), control.H());
				button->copy_label(Text(node, "text").c_str());
				FltkTheme::For(control, "CheckBox", node, "", FltkTheme::SizeLabel).Apply(button);
				button->callback(CheckChanged, static_cast<CheckBoxControl *>(&control));

				return new FltkCheck(button);
			}

			ControlView *MakeLabel(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_Box *box = new Fl_Box(control.X(), control.Y(), control.W(), control.H());
				box->copy_label(Text(node, "text").c_str());
				FltkTheme::For(control, "Label", node, static_cast<LabelControl &>(control).Indexed() ? "indexed" : "",
					FltkTheme::SizeLabel)
					.Apply(box);
				const std::string align = Text(node, "align");
				const bool clip = node.Get("clip") && node.Get("clip")->AsBool();
				box->align(FL_ALIGN_INSIDE | (clip ? FL_ALIGN_CLIP : 0) |
					(align == "left" ? FL_ALIGN_LEFT : align == "right" ? FL_ALIGN_RIGHT : FL_ALIGN_CENTER));

				return new FltkLabel(box);
			}

			ControlView *MakeRadioButton(PanelControl &control, const Spumoni::JSON &node)
			{
				static const struct
				{
					const char *name;
					unsigned char **image;
				} glyphs[] = {{"sine", image_Sine}, {"triangle", image_Tri}, {"square", image_Square}, {"saw", image_Saw},
					{"reverse_saw", image_RevSaw}, {"pulse1", image_Pulse1}, {"pulse2", image_Pulse2},
					{"inverse_sine", image_InvSine}, {"noise", image_Noise}};
				Fl_LED_Button *button = new Fl_LED_Button(control.X(), control.Y(), control.W(), control.H());
				button->type(FL_RADIO_BUTTON);
				const std::string glyph = Text(node, "glyph");
				Fl_Pixmap *pixmap = NULL;

				for (size_t i = 0; i < sizeof glyphs / sizeof glyphs[0] && !pixmap; ++i)
				{
					if (glyph == glyphs[i].name)
						pixmap = new Fl_Pixmap(glyphs[i].image);
				}

				const FltkLook look = FltkTheme::For(control, "RadioButton", node, pixmap ? "glyph" : "text");
				look.Apply(button);

				if (look.Has("down_box"))
					button->down_box(look.Box("down_box"));

				if (pixmap)
					pixmap->label(button);
				else
				{
					button->copy_label(Text(node, "text").c_str());
					button->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
				}

				button->callback(CheckChanged, static_cast<CheckBoxControl *>(&control));

				return new FltkCheck(button, pixmap);
			}

			void SpinnerChanged(Fl_Widget *widget, void *data)
			{
				static_cast<SpinnerControl *>(data)->Changed(static_cast<Fl_Valuator *>(widget)->value());
			}

			ControlView *MakeSpinner(PanelControl &control, const Spumoni::JSON &node)
			{
				const Spumoni::JSON *simple = node.Get("simple");
				Counter *counter = new Counter(control.X(), control.Y(), control.W(), control.H());
				counter->type(!simple || simple->AsBool() ? FL_SIMPLE_COUNTER : FL_NORMAL_COUNTER);
				const FltkLook look = FltkTheme::For(control, "Spinner", node, "", FltkTheme::SizeText);
				look.Apply(counter);
				look.ApplyText(counter);
				counter->callback(SpinnerChanged, static_cast<SpinnerControl *>(&control));

				return new FltkSpinner(counter);
			}

			void TextChanged(Fl_Widget *widget, void *data)
			{
				static_cast<TextEntryControl *>(data)->Changed(static_cast<Fl_Input *>(widget)->value());
			}

			ControlView *MakeTextEntry(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_Input *input = new Fl_Input(control.X(), control.Y(), control.W(), control.H());
				const FltkLook look = FltkTheme::For(control, "TextEntry", node);
				look.Apply(input);
				look.ApplyText(input);

				input->when(FL_WHEN_ENTER_KEY | FL_WHEN_RELEASE);
				input->callback(TextChanged, static_cast<TextEntryControl *>(&control));

				return new FltkText(input);
			}

			void ChoiceChanged(Fl_Widget *widget, void *data)
			{
				static_cast<ChoiceControl *>(data)->Changed(static_cast<Fl_Choice *>(widget)->value());
			}

			ControlView *MakeChoice(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_Choice *choice = new Fl_Choice(control.X(), control.Y(), control.W(), control.H());
				const FltkLook look = FltkTheme::For(control, "Choice", node, "", FltkTheme::SizeLabel | FltkTheme::SizeText);
				look.Apply(choice);
				look.ApplyText(choice);
				choice->callback(ChoiceChanged, static_cast<ChoiceControl *>(&control));

				return new FltkChoice(choice);
			}

			ControlView *MakeButton(PanelControl &control, const Spumoni::JSON &node)
			{
				ButtonControl &owner = static_cast<ButtonControl &>(control);
				const Spumoni::JSON *repeat = node.Get("repeat");
				const bool repeats = repeat && (repeat->GetType() == Spumoni::JSON::Object ||
						(repeat->GetType() == Spumoni::JSON::Boolean && repeat->AsBool()));
				Fl_Button *button = repeats ? new Fl_Repeat_Button(control.X(), control.Y(), control.W(), control.H())
						: new Fl_Button(control.X(), control.Y(), control.W(), control.H());
				button->copy_label(owner.Label().c_str());
				FltkTheme::For(control, "Button", node, "", FltkTheme::SizeLabel).Apply(button);

				if (Text(node, "label") == "symbol")
					button->labeltype(FL_SYMBOL_LABEL);

				if (owner.ButtonMode() == ButtonControl::Radio)
					button->type(FL_RADIO_BUTTON);
				else if (owner.ButtonMode() == ButtonControl::Toggle)
					button->type(FL_TOGGLE_BUTTON);

				if (node.Get("pressed") && node.Get("pressed")->AsBool())
					button->value(1);

				button->callback(ButtonChanged, &owner);

				return new FltkButton(button);
			}

			void ListClicked(Fl_Widget *, void *data)
			{
				static_cast<ListChoosingButtonControl *>(data)->Clicked();
			}

			ControlView *MakeListChoosingButton(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_Button *button = new Fl_Button(control.X(), control.Y(), control.W(), control.H());
				button->copy_label(Text(node, "text").c_str());
				FltkTheme::For(control, "ListChoosingButton", node, "", FltkTheme::SizeLabel).Apply(button);
				button->callback(ListClicked, static_cast<ListChoosingButtonControl *>(&control));

				return new FltkButton(button);
			}

			void FileClicked(Fl_Widget *widget, void *data)
			{
				static_cast<FileChoosingButtonControl *>(data)->Clicked(static_cast<Fl_Button *>(widget)->value() != 0);
			}

			ControlView *MakeFileChoosingButton(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_Button *button = new Fl_Button(control.X(), control.Y(), control.W(), control.H());
				button->copy_label(Text(node, "text").c_str());
				FltkTheme::For(control, "FileChoosingButton", node, "", FltkTheme::SizeLabel).Apply(button);

				if (control.Toggles())
					button->type(FL_TOGGLE_BUTTON);

				button->callback(FileClicked, static_cast<FileChoosingButtonControl *>(&control));

				return new FltkButton(button);
			}

			ControlView *MakeNotebook(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_Tabs *tabs = new Fl_Tabs(control.X(), control.Y(), control.W(), control.H());
				FltkTheme::For(control, "Notebook", node).Apply(tabs);

				return new FltkTabs(tabs, FltkTheme::For(control, "NotebookPage", node));
			}

			bool Flag(const Spumoni::JSON *node, bool fallback)
			{
				if (!node)
					return fallback;

				if (node->GetType() == Spumoni::JSON::Boolean)
					return node->AsBool();

				return PanelControl::NumberOf(node, fallback ? 1 : 0) != 0;
			}

			ControlView *MakeScrollArea(PanelControl &control, const Spumoni::JSON &node)
			{
				Fl_VirtualScrollArea *area = new Fl_VirtualScrollArea(control.X(), control.Y(), control.W(), control.H());
				area->Show(Flag(node.Get("horizontal"), true), Flag(node.Get("vertical"), false),
					Flag(node.Get("always"), false));
				FltkTheme::For(control, "VirtualScrollArea", node).Apply(area);

				return new FltkScrollArea(area);
			}

			ViewRegistry::Entry s_Knob("Knob", &MakeKnob);
			ViewRegistry::Entry s_Slider("Slider", &MakeSlider);
			ViewRegistry::Entry s_CheckBox("CheckBox", &MakeCheckBox);
			ViewRegistry::Entry s_Label("Label", &MakeLabel);
			ViewRegistry::Entry s_RadioButton("RadioButton", &MakeRadioButton);
			ViewRegistry::Entry s_Spinner("Spinner", &MakeSpinner);
			ViewRegistry::Entry s_TextEntry("TextEntry", &MakeTextEntry);
			ViewRegistry::Entry s_Choice("Choice", &MakeChoice);
			ViewRegistry::Entry s_Button("Button", &MakeButton);
			ViewRegistry::Entry s_ListChoosingButton("ListChoosingButton", &MakeListChoosingButton);
			ViewRegistry::Entry s_FileChoosingButton("FileChoosingButton", &MakeFileChoosingButton);
			ViewRegistry::Entry s_Notebook("Notebook", &MakeNotebook);
			ViewRegistry::Entry s_ScrollArea("VirtualScrollArea", &MakeScrollArea);
		}
	}
}
