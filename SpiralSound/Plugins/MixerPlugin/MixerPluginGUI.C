/*  SpiralPlugin
 *  Copyleft (C) 2000 David Griffiths <dave@pawfal.org>
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

#include "MixerPluginGUI.h"
#include <FL/fl_draw.H>
#include <FL/fl_draw.H>

using namespace std;

const float default_slider_value = 1.0f;

////////////////////////////////////////////

MixerPluginGUI::ChanGUI::ChanGUI (int ChanNum, MixerPluginGUI *p, Fl_Color SelColour) {
    m_SliderGroup = new Fl_Group (0, 0, 20, 108, "");
    m_SliderGroup->user_data ((void *)p);

    m_PeakInd = new Fl_LED_Button (0, -8, 20, 20, "");
    m_SliderGroup->add (m_PeakInd);

    m_Chan = new Fl_Slider (0, 8, 20, 100, "");
    m_Chan->type (FL_VERT_NICE_SLIDER);
    m_Chan->box (FL_PLASTIC_DOWN_BOX);
    m_Chan->selection_color (SelColour);
    m_Chan->maximum (2);
    m_Chan->step (0.01);
    m_Chan->value (default_slider_value);
    m_Chan->labelsize (10);
    m_Chan->callback ((Fl_Callback*)MixerPluginGUI::cb_Chan, (void*)&Numbers[ChanNum]);
    m_SliderGroup->add(m_Chan);
}

////////////////////////////////////////////

MixerPluginGUI::MixerPluginGUI (int w, int h, MixerPlugin *o, ChannelHandler *ch, const HostInfo *Info) :
SpiralPluginGUI (w, h, o, ch)
{
        m_GUIColour = (Fl_Color)Info->GUI_COLOUR;
        for (int c=0; c<MAX_CHANNELS; c++) Numbers[c]=c;

	m_MainPack = new Fl_Pack (0, 15, w, 108);
	m_MainPack->type (FL_HORIZONTAL);
        add (m_MainPack);

        // start with four...
	AddChan(); AddChan(); AddChan(); AddChan();

        m_Buttons = new Fl_Pack (0, 126, 62, 20);
        m_Buttons->type (FL_HORIZONTAL);
        add (m_Buttons);

        m_Delete = new Fl_Button (2, 0, 20, 20, "-");
        m_Delete->user_data ((void*)(this));
        m_Delete->box (FL_PLASTIC_UP_BOX);
        m_Delete->color (Info->GUI_COLOUR);
        m_Delete->selection_color (Info->GUI_COLOUR);
	m_Delete->callback ((Fl_Callback*)cb_Delete);
	m_Buttons->add (m_Delete);

        m_Add = new Fl_Button (22, 0, 20, 20, "+");
        m_Add->user_data ((void*)(this));
        m_Add->box (FL_PLASTIC_UP_BOX);
        m_Add->color (Info->GUI_COLOUR);
        m_Add->selection_color (Info->GUI_COLOUR);
	m_Add->callback ((Fl_Callback*)cb_Add);
	m_Buttons->add (m_Add);

        m_PeakInd = new Fl_LED_Button (42, 0, 20, 20, "");
        m_Buttons->add (m_PeakInd);
}

void MixerPluginGUI::AddChan (bool SendData, bool ResizeIt) {
     int num = (int)m_GUIVec.size();
     ChanGUI *NewChan = new ChanGUI (num, this, m_GUIColour);
     m_GUIVec.push_back (NewChan);
     m_MainPack->add (NewChan->m_SliderGroup);
     if (SendData) {
        m_GUICH->SetCommand (MixerPlugin::ADDCHAN);
        m_GUICH->Wait ();
        m_GUICH->Set ("Num", ++num);
        m_GUICH->Set ("Value", (float)(2.0f - default_slider_value));
        m_GUICH->SetCommand(MixerPlugin::SETMIX);
        m_GUICH->Wait ();
     }
     if (ResizeIt && num > 3) Resize (w()+20, h());
}

void MixerPluginGUI::DeleteChan (bool SendData) {
     vector<ChanGUI*>::iterator i = m_GUIVec.end();
     i--;
     m_MainPack->remove ((*i)->m_SliderGroup);
     delete *i;
     m_GUIVec.erase (i);
     int num = (int)m_GUIVec.size();
     if (SendData) {
        m_GUICH->SetCommand (MixerPlugin::REMOVECHAN);
        m_GUICH->Wait ();
     }
     if (num > 2) Resize (w()-20, h());
}

void MixerPluginGUI::UpdateValues(SpiralPlugin *o) {
     MixerPlugin *Plugin = (MixerPlugin *)o;
     unsigned int chans = Plugin->GetChannels();
     while (chans < m_GUIVec.size()) DeleteChan (false);
     while (chans > m_GUIVec.size()) AddChan (false, true);
     for (unsigned int c=0; c<chans; c++)
         m_GUIVec[c]->m_Chan->value (2.0f - Plugin->GetChannel (c));
     redraw();
}

void MixerPluginGUI::Update () {
     if (m_GUICH->GetBool ("Peak")) m_PeakInd->value (true);
     m_GUICH->GetData ("inPeak", m_inPeak);
     for (unsigned int c=0; c<m_GUIVec.size(); c++) {
         if (m_inPeak[c]) m_GUIVec[c]->m_PeakInd->value (true);
     }
}

inline void MixerPluginGUI::cb_Add_i (Fl_Button* o, void* v) {
       m_PeakInd->value (false);
       if ((int)m_GUIVec.size() < MAX_CHANNELS) AddChan (true, true);
}

void MixerPluginGUI::cb_Add (Fl_Button* o, void* v) {
     ((MixerPluginGUI*)(o->user_data()))->cb_Add_i (o, v);
}

inline void MixerPluginGUI::cb_Delete_i (Fl_Button* o, void* v) {
       m_PeakInd->value (false);
       if (m_GUIVec.size() > 2) DeleteChan ();
}

void MixerPluginGUI::cb_Delete (Fl_Button* o, void* v) {
     ((MixerPluginGUI*)(o->user_data()))->cb_Delete_i (o, v);
}

inline void MixerPluginGUI::cb_Chan_i (Fl_Slider* o, void* v) {
       int num = (*(int*)(v));
       m_PeakInd->value (false);
       m_GUIVec[num]->m_PeakInd->value (false);
       m_GUICH->Set("Num", num);
       m_GUICH->Set("Value", (float)(2.0f-o->value()));
       m_GUICH->SetCommand (MixerPlugin::SETMIX);
}

void MixerPluginGUI::cb_Chan(Fl_Slider* o, void* v) {
     ((MixerPluginGUI*)(o->parent()->user_data()))->cb_Chan_i (o, v);
}

const string MixerPluginGUI::GetHelpText (const string &loc){
      return string("")
      + "A general purpose mixer.\n"
      + "Useful for mixing CV values as well as mono audio\n"
      + "signals.\n"
      + "The LEDs indicate the the mixer inputs or output is\n"
      + "at peak level, click on them, or change appropriate\n"
      + "levels to reset them.\n"
      + "Add up to 16 channels using the '+' button.\n"
      + "Use the '-' button to remove unwanted channels.\n";
}
