// SPDX-License-Identifier: GPL-2.0-or-later
#include "Patch.h"
#include "FltkTheme.h"
#include "FltkView.h"
#include "Fl_VirtualScrollArea.H"
#include "JSON.h"
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input_.H>
#include <FL/fl_draw.H>
#include <cstdio>
#include <vector>

namespace Spiral
{
	namespace Panels
	{
		namespace
		{
			class FltkKeys;

			class PianoKeysGroup: public Fl_Group
			{
			public:
				PianoKeysGroup(int x, int y, int w, int h): Fl_Group(x, y, w, h), Keys(NULL) {}

				int handle(int event);
				void draw();

				FltkKeys *Keys;
			};

			class FltkKeys: public FltkView, public virtual KeysView
			{
			public:
				FltkKeys(PianoKeysGroup *group, const std::vector<Fl_Button *> &keys, PianoKeysControl &control,
						const FltkLook &look):
					FltkView(group), m_Group(group), m_Keys(keys), m_Control(control), m_Look(look)
				{
					group->Keys = this;
				}

				Fl_Button *Key(int note) const
				{
					const int index = note - m_Control.First();

					return index >= 0 && index < (int)m_Keys.size() ? m_Keys[index] : NULL;
				}

				void SetKey(int index, bool down)
				{
					if (index < 0 || index >= (int)m_Keys.size() || !m_Keys[index])
						return;

					m_Keys[index]->value(down ? 1 : 0);
					m_Keys[index]->redraw();
				}

				void Frame(bool focused, int firstNote, int lastNote)
				{
					for (size_t n = 0; n < m_Keys.size(); ++n)
					{
						const int note = m_Control.First() + (int)n;
						const Fl_Color base = m_Look.Colour(PianoKeysControl::IsBlack(note) ? "black" : "white");

						if (m_Keys[n])
							m_Keys[n]->color(focused && note >= firstNote && note <= lastNote ?
								fl_color_average(m_Look.Colour("focus"), base, m_Look.Size("focus_percent") / 100.0f) : base);
					}

					m_Group->redraw();
				}

				bool Focused() const
				{
					return Fl::focus() && Fl::focus()->inside(m_Group);
				}

				void ShowNote(int note)
				{
					Fl_VirtualScrollArea *area = dynamic_cast<Fl_VirtualScrollArea *>(m_Group->parent());
					const Fl_Button *key = Key(note);

					if (area && key)
						area->SetView(area->ViewX() + key->x() - area->x(), 0);
				}

				bool Typed(int event)
				{
					if (!m_Control.ComputerKeys())
						return false;

					switch (event)
					{
						case FL_ENTER:
							if (!dynamic_cast<Fl_Input_ *>(Fl::focus()))
								Fl::focus(m_Group);

							m_Control.Entered();

							return true;
						case FL_FOCUS:
							return true;
						case FL_UNFOCUS:
							m_Control.Unfocused();

							return true;
						case FL_KEYDOWN:
						case FL_KEYUP:
							break;
						default:
							return false;
					}

					const int key = Fl::event_key();

					if (event == FL_KEYDOWN && (Fl::event_state() & (FL_CTRL | FL_ALT | FL_META)))
						return false;

					if (event == FL_KEYDOWN && key >= FL_F + 1 && key <= FL_F + 11)
					{
						m_Control.SelectOctave(key - (FL_F + 1));

						return true;
					}

					return key > 0 && key < 128 && m_Control.Typed((char)key, event == FL_KEYDOWN);
				}

				void DrawOverlay()
				{
					if (!m_Control.ComputerKeys() || !Focused())
						return;

					const int first = m_Control.Octave() * 12;
					const int last = m_Control.LastMapped();
					const Fl_Button *from = Key(first);
					const Fl_Button *to = Key(last);

					if (!from || !to)
						return;

					fl_color(m_Look.Colour("focus"));
					fl_line_style(FL_SOLID, 2);
					fl_rect(from->x() + 1, m_Group->y() + 1, to->x() + to->w() - from->x() - 2, m_Group->h() - 2);
					fl_line_style(0);

					for (int i = 0; i < PianoKeysControl::KeyMapSize(); ++i)
					{
						const int note = first + i;
						const Fl_Button *key = Key(note);

						if (!key)
							break;

						const bool black = PianoKeysControl::IsBlack(note);
						char label[3] = {PianoKeysControl::KeyMap()[i], 0, 0};

						if (note == m_Control.First() + m_Control.Count() - 1 && i < PianoKeysControl::KeyMapSize() - 1)
							label[1] = '+';

						fl_font(m_Look.Font("map_font"),
							m_Look.Size(label[1] ? "map_size_last" : black ? "map_size_black" : "map_size_white"));
						fl_color(m_Look.Colour(black ? "map_text_black" : "map_text_white"));
						fl_draw(label, key->x(), key->y() + key->h() - (black ? 14 : 24), key->w(), 12, FL_ALIGN_CENTER);
					}
				}

				void Changed(Fl_Widget *widget)
				{
					for (size_t n = 0; n < m_Keys.size(); ++n)
					{
						if (m_Keys[n] == widget)
						{
							m_Control.KeyChanged((int)n, static_cast<Fl_Button *>(widget)->value() != 0);

							return;
						}
					}
				}

			private:
				PianoKeysGroup *m_Group;
				std::vector<Fl_Button *> m_Keys;
				PianoKeysControl &m_Control;
				FltkLook m_Look;
			};

			int PianoKeysGroup::handle(int event)
			{
				const int used = Fl_Group::handle(event);

				return (Keys && Keys->Typed(event)) || used;
			}

			void PianoKeysGroup::draw()
			{
				if (!(damage() & ~FL_DAMAGE_CHILD))
				{
					for (int i = 0; i < children(); ++i)
					{
						const Fl_Widget *white = child(i);

						if (!white->damage())
							continue;

						for (int j = i + 1; j < children(); ++j)
						{
							Fl_Widget *black = child(j);

							if (black->x() < white->x() + white->w() && white->x() < black->x() + black->w() &&
							    black->y() < white->y() + white->h() && white->y() < black->y() + black->h())
								black->clear_damage(black->damage() | FL_DAMAGE_ALL);
						}
					}
				}

				Fl_Group::draw();

				if (Keys)
					Keys->DrawOverlay();
			}

			void KeyChanged(Fl_Widget *widget, void *data)
			{
				PianoKeysGroup *group = static_cast<PianoKeysGroup *>(data);

				if (group->Keys)
					group->Keys->Changed(widget);
			}

			Fl_Button *MakeKey(int x, int y, int w, int h, bool toggle, bool black, const FltkLook &look,
					PianoKeysGroup *group)
			{
				Fl_Button *key = new Fl_Button(x, y, w, h, "");
				const Fl_Color colour = look.Colour(black ? "black" : "white");
				key->box(look.Box("key_box"));
				key->labelsize(look.Size("key_label_size"));
				key->when(FL_WHEN_CHANGED);
				key->color(colour);
				key->selection_color(toggle ? look.Colour("pressed") : colour);

				if (toggle)
					key->type(FL_TOGGLE_BUTTON);

				key->callback(KeyChanged, group);

				return key;
			}

			ControlView *MakePianoKeys(PanelControl &control, const Spumoni::JSON &node)
			{
				PianoKeysControl &piano = static_cast<PianoKeysControl &>(control);
				const int keys = piano.Count();
				const int first = piano.First();
				const bool toggle = piano.Toggles();
				const int keyWidth = (int)PanelControl::NumberOf(node.Get("white_key_width"), 10);
				const int blackWidth = (int)PanelControl::NumberOf(node.Get("black_key_width"), keyWidth);
				const int blackHeight = (int)PanelControl::NumberOf(node.Get("black_key_height"), control.H() * 3 / 5);
				const int blackOffset = (int)PanelControl::NumberOf(node.Get("black_key_offset"), keyWidth / 2);
				const bool labels = !node.Get("labels") || node.Get("labels")->AsBool();
				int whites = 0;

				for (int n = 0; n < keys; ++n)
				{
					if (!PianoKeysControl::IsBlack(first + n))
						++whites;
				}

				PianoKeysGroup *group = new PianoKeysGroup(control.X(), control.Y(), whites * keyWidth, control.H());
				const FltkLook look = FltkTheme::For(control, "PianoKeys", node);
				look.Apply(group);
				std::vector<Fl_Button *> made(keys > 0 ? keys : 0, (Fl_Button *)NULL);
				int count = 0;

				for (int n = 0; n < keys; ++n)
				{
					const int note = first + n;

					if (PianoKeysControl::IsBlack(note))
						continue;

					Fl_Button *key = MakeKey(control.X() + count * keyWidth, control.Y(), keyWidth, control.H(), toggle, false,
						look, group);
					count++;

					if (labels && note % 12 == 0)
					{
						char label[8];
						std::snprintf(label, sizeof(label), "%d", note / 12);
						key->copy_label(label);
						key->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);
					}

					made[n] = key;
				}

				int pos = 0;
				count = 0;

				for (int n = 0; n < keys; ++n)
				{
					if (!PianoKeysControl::IsBlack(first + n))
					{
						pos = count * keyWidth;
						count++;

						continue;
					}

					made[n] = MakeKey(control.X() + pos + blackOffset, control.Y(), blackWidth, blackHeight, toggle, true,
						look, group);
				}

				group->end();

				return new FltkKeys(group, made, piano, look);
			}

			ViewRegistry::Entry s_PianoKeys("PianoKeys", &MakePianoKeys);
		}
	}
}
