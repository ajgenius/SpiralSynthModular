/*  MousePlugin
 *  Copyleft (C) 2002 Dan Bethell <dan@pawfal.org>
 *                    Dave Griffiths <dave@pawfal.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "MousePluginGUI.h"
#include <FL/fl_draw.h>
#include <FL/fl_draw.H>

ScratchWidget::ScratchWidget (int x, int y, int w, int h, const char *l) :
Fl_Widget (x, y, w, h, l),
m_Data (0.0)
{
}

void ScratchWidget::draw() {
     fl_color (color ());
     fl_rectf (x(), y(), w(), h());
     fl_push_clip (x(), y(), w(), h());
     int val = (int)(((m_Data * -1) + 1) * (h() / 2.0));
     fl_color (selection_color ());
     fl_rectf (x(), y() + val, w(), h());
     fl_pop_clip();
}

////////////////////////////////////////////

MousePluginGUI::MousePluginGUI (int w, int h, MousePlugin *o, ChannelHandler *ch, const HostInfo *Info) :
SpiralPluginGUI (w, h, o, ch)
{
    m_Port0 = new Fl_LED_Button (0, 10, 23, 23);
    m_Port0->type (FL_RADIO_BUTTON);
    m_Port0->labelsize (10);
    m_Port0->label ("ttyS0");
    m_Port0->set();
    m_Port0->callback ((Fl_Callback*)cb_Port0);

    m_Port1 = new Fl_LED_Button (50, 10, 23, 23);
    m_Port1->type (FL_RADIO_BUTTON);
    m_Port1->labelsize (10);
    m_Port1->label ("ttyS1");
    m_Port1->callback ((Fl_Callback*)cb_Port1);

    m_Scope = new ScratchWidget (5, 40, 60, 100, "Scratch");
    m_Scope->color(Info->SCOPE_BG_COLOUR);
    m_Scope->selection_color(Info->SCOPE_FG_COLOUR);

    end();
}

void MousePluginGUI::Update () {
     m_Scope->m_Data = m_GUICH->GetFloat ("Data");
     m_Scope->redraw();
}

// Update GUI on Load
void MousePluginGUI::UpdateValues (SpiralPlugin* o) {
}

inline void MousePluginGUI::cb_Port0_i (Fl_LED_Button* o, void* v) {
       m_GUICH->Set("Port", '0');
       m_GUICH->SetCommand (MousePlugin::SETPORT);
}

void MousePluginGUI::cb_Port0 (Fl_LED_Button* o, void* v) {
     ((MousePluginGUI*)(o->parent ()))->cb_Port0 (o, v);
}

inline void MousePluginGUI::cb_Port1_i (Fl_LED_Button* o, void* v) {
       m_GUICH->Set("Port", '1');
       m_GUICH->SetCommand (MousePlugin::SETPORT);
}

void MousePluginGUI::cb_Port1 (Fl_LED_Button* o, void* v) {
     ((MousePluginGUI*)(o->parent ()))->cb_Port1 (o, v);
}

const string MousePluginGUI::GetHelpText (const string &loc) {
      return string("")
      + "\n"
      + "\n";
}
