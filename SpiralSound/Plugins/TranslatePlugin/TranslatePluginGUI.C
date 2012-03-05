/*  SpiralPlugin
 *  Copyleft (C) 2003 Andy Preston <andy@clublinux.co.uk>
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

#include "TranslatePluginGUI.h"
#include <FL/fl_draw.H>

using namespace std;

////////////////////////////////////////////

TranslatePluginGUI::TranslatePluginGUI (int w, int h,TranslatePlugin *o,ChannelHandler *ch,const HostInfo *Info) :
SpiralPluginGUI (w, h, o, ch)
{
     m_Method = new Fl_Choice (0, 20, 150, 20, "");
     //m_Method->color (Info->GUI_COLOUR);
     m_Method->box (FL_PLASTIC_UP_BOX);
     m_Method->textsize (10);
     m_Method->labelsize (10);
     m_Method->callback ((Fl_Callback*) cb_Method);
     m_Method->add ("Pass Through");
     m_Method->add ("Note -> Frequency");
     m_Method->add ("Frequency -> Note");
     m_Method->add ("Note -> Note Number");
     m_Method->add ("Note Number -> Note");
     m_Method->value (0);
     add (m_Method);
     end ();
}

void TranslatePluginGUI::UpdateValues (SpiralPlugin *o) {
     TranslatePlugin *Plugin = (TranslatePlugin *)o;
     m_Method->value (Plugin->GetMethod ());
}

inline void TranslatePluginGUI::cb_Method_i (Fl_Choice *o, void *v) {
    m_GUICH->Set ("Method", int (o->value ()));
    m_GUICH->SetCommand (TranslatePlugin::SETMETHOD);
}

void TranslatePluginGUI::cb_Method (Fl_Choice *o, void *v) {
    ((TranslatePluginGUI*)(o->parent ()))->cb_Method_i (o, v);
}

const string TranslatePluginGUI::GetHelpText (const string &loc) {
    return string("")
    + "Translates control voltages into other formats.";
}
