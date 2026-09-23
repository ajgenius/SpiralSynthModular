/*  SpiralLoops
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

#include "ReverbGUI.h"

static const int GUI_COLOUR = 154;
static const int GUIBG_COLOUR = 144;
static const int GUIBG2_COLOUR = FL_GRAY;

ReverbGUI::ReverbGUI(Reverb *o)
{
	m_reverb=o;
	if (!m_reverb) cerr<<"WARNING: Reverb not correctly set up"<<endl;
}

ReverbGUI::~ReverbGUI()
{	
	delete GUIReverbGroup;
}
		
void ReverbGUI::CreateGUI(int xoff, int yoff, char *name)
{
	 Fl_Group* o = GUIReverbGroup = new Fl_Group(xoff, yoff, 300, 60, name);
      o->type(1);
	  o->color(GUIBG2_COLOUR);
	  o->box(FL_UP_BOX);
      o->labeltype(FL_ENGRAVED_LABEL);
      o->align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE);
	  o->user_data((void*)(this));
       	  
	  Time = new Fl_Knob(xoff+50, yoff+5, 40, 40, "Time");
	  Time->color(GUI_COLOUR);
      Time->labelsize(10);
	  Time->maximum(0.25);
      Time->step(0.01);
      Time->value(0.0);
      Time->callback((Fl_Callback*)cb_Time);
	  
	  FeedBack = new Fl_Knob(xoff+100, yoff+5, 40, 40, "FeedBack");
	  FeedBack->color(GUI_COLOUR);
      FeedBack->labelsize(10);
	  FeedBack->maximum(1);
      FeedBack->step(0.01);
      FeedBack->value(0.01);
      FeedBack->callback((Fl_Callback*)cb_Feedback);
	  
      Randomness = new Fl_Knob(xoff+150, yoff+5, 40, 40, "Random");
	  Randomness->color(GUI_COLOUR);
      Randomness->labelsize(10);
	  Randomness->maximum(0.01);
      Randomness->step(0.00001);
      Randomness->value(0.002);
      Randomness->callback((Fl_Callback*)cb_Randomness);
	  
	  Bypass = new Fl_Button(xoff+5, yoff+25, 40, 20, "Bypass");
	  Bypass->color(GUIBG2_COLOUR);
	  Bypass->labelsize(10);
	  Bypass->type(1);
	  Bypass->value(1);
	  Bypass->callback((Fl_Callback*)cb_Bypass);
	  
   	o->end();
    
}

void ReverbGUI::UpdateValues()
{
	Time->value(m_reverb->GetTime());
	FeedBack->value(m_reverb->GetFeedback());
	Randomness->value(m_reverb->GetRandomness());
	Bypass->value(m_reverb->IsBypassed());
}

//// Callbacks ////

inline void ReverbGUI::cb_Time_i(Fl_Knob* o, void* v) 
{ m_reverb->SetTime(o->value()); }
void ReverbGUI::cb_Time(Fl_Knob* o, void* v) 
{ ((ReverbGUI*)(o->parent()->user_data()))->cb_Time_i(o,v); }

inline void ReverbGUI::cb_Feedback_i(Fl_Knob* o, void* v) 
{ m_reverb->SetFeedback(o->value()); }
void ReverbGUI::cb_Feedback(Fl_Knob* o, void* v) 
{ ((ReverbGUI*)(o->parent()->user_data()))->cb_Feedback_i(o,v); }

inline void ReverbGUI::cb_Randomness_i(Fl_Knob* o, void* v) 
{ m_reverb->SetRandomness(o->value()); }
void ReverbGUI::cb_Randomness(Fl_Knob* o, void* v) 
{ ((ReverbGUI*)(o->parent()->user_data()))->cb_Randomness_i(o,v); }

inline void ReverbGUI::cb_Bypass_i(Fl_Button* o, void* v) 
{ m_reverb->SetBypass(o->value()); }
void ReverbGUI::cb_Bypass(Fl_Button* o, void* v) 
{ ((ReverbGUI*)(o->parent()->user_data()))->cb_Bypass_i(o,v); }
