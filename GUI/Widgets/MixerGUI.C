/*  SpiralSynth
 *  Copyright (C) 2000 David Griffiths <dave@blueammonite.f9.co.uk>
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

#include "MixerGUI.h"

MixerGUI::MixerGUI(Mixer *o)
{
	m_mix=o;
	if (!m_mix) cerr<<"WARNING: Mixer not correctly set up"<<endl;
}
		
void MixerGUI::CreateGUI(int xoff=0, int yoff=0, char *name)
{
	 Fl_Group* o = GUIMixGroup = new Fl_Group(xoff, yoff, 100, 110, name);
      o->type(1);
	  o->box(FL_UP_BOX);
      o->labeltype(FL_ENGRAVED_LABEL);
      o->align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE);
	  o->user_data((void*)(this));
       	  
	  Balance = new Fl_Knob(xoff+55, yoff+35, 40, 40, "Amount");
      Balance->color(SynthInfo::GUI_COLOUR);
	  Balance->type(Fl_Knob::LINELIN);
      Balance->labelsize(10); 
      Balance->maximum(0.002);
      Balance->step(0.00001);
      Balance->value(0.001);   
	  Balance->callback((Fl_Callback*)cb_Balance);

	  Add = new Fl_Check_Button(xoff, yoff+15, 55, 30, "Add");
      Add->type(102);
      Add->down_box(FL_DIAMOND_DOWN_BOX);
      Add->selection_color(SynthInfo::GUI_COLOUR);
      Add->set();      
	  Add->callback((Fl_Callback*)cb_Add);
	 
	  XMod = new Fl_Check_Button(xoff, yoff+40, 55, 30, "XM");
      XMod->type(102);
      XMod->down_box(FL_DIAMOND_DOWN_BOX);
      XMod->selection_color(SynthInfo::GUI_COLOUR);   
	  XMod->callback((Fl_Callback*)cb_XMod);
	  
      Ring = new Fl_Check_Button(xoff, yoff+65, 55, 30, "Ring");
      Ring->type(102);
      Ring->down_box(FL_DIAMOND_DOWN_BOX);
      Ring->selection_color(SynthInfo::GUI_COLOUR);   
	  Ring->callback((Fl_Callback*)cb_Ring);
     
      o->end();
    
}

void MixerGUI::UpdateValues()
{		 
	Balance->value(m_mix->GetAmount());
	
	Add->value(0);
	XMod->value(0);
	Ring->value(0);
	
	switch(m_mix->GetType())
	{
		case Mixer::ADD: Add->value(1); break;
		case Mixer::XMOD: XMod->value(1); break;
		case Mixer::RING: Ring->value(1); break;
		case Mixer::NONE: break;
	}
	
}

//// Callbacks ////

inline void MixerGUI::cb_Balance_i(Fl_Knob* o, void* v)
{ m_mix->SetAmount(o->value()); }
void MixerGUI::cb_Balance(Fl_Knob* o, void* v)
{ ((MixerGUI*)(o->parent()->user_data()))->cb_Balance_i(o,v); }

inline void MixerGUI::cb_Add_i(Fl_Check_Button* o, void* v)
{ m_mix->SetType(Mixer::ADD); }
void MixerGUI::cb_Add(Fl_Check_Button* o, void* v)
{ ((MixerGUI*)(o->parent()->user_data()))->cb_Add_i(o,v); }

inline void MixerGUI::cb_XMod_i(Fl_Check_Button* o, void* v)
{ m_mix->SetType(Mixer::XMOD); }
void MixerGUI::cb_XMod(Fl_Check_Button* o, void* v) 
{ ((MixerGUI*)(o->parent()->user_data()))->cb_XMod_i(o,v); }

inline void MixerGUI::cb_Ring_i(Fl_Check_Button* o, void* v)
{ m_mix->SetType(Mixer::RING); }
void MixerGUI::cb_Ring(Fl_Check_Button* o, void* v) 
{ ((MixerGUI*)(o->parent()->user_data()))->cb_Ring_i(o,v); }

