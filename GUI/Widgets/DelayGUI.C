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

#include "DelayGUI.h"

static const int GUI_COLOUR = 154;
static const int GUIBG_COLOUR = 144;
static const int GUIBG2_COLOUR = FL_GRAY;

DelayGUI::DelayGUI(Delay *o)
{
	m_delay=o;
	if (!m_delay) cerr<<"WARNING: Delay not correctly set up"<<endl;
}

DelayGUI::~DelayGUI()
{
	delete GUIDelayGroup;
}
		
void DelayGUI::CreateGUI(int xoff, int yoff, char *name)
{
	 Fl_Group* o = GUIDelayGroup = new Fl_Group(xoff, yoff, 300, 60, name);
      o->type(1);
	  o->color(GUIBG2_COLOUR);
	  o->box(FL_UP_BOX);
      o->labeltype(FL_ENGRAVED_LABEL);
      o->align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE);
	  o->user_data((void*)(this));
       	  
	  DelayA = new Fl_Knob(xoff+50, yoff+5, 40, 40, "Delay");
	  DelayA->color(GUI_COLOUR);
      DelayA->labelsize(10);
	  DelayA->maximum(1);
      DelayA->step(0.01);
      DelayA->value(0.5);
      DelayA->callback((Fl_Callback*)cb_Delay);
	  
	  Feedback = new Fl_Knob(xoff+100, yoff+5, 40, 40, "Feedback");
	  Feedback->color(GUI_COLOUR);
      Feedback->labelsize(10);
	  Feedback->maximum(1.0);
      Feedback->step(0.01);
      Feedback->value(0.5);
      Feedback->callback((Fl_Callback*)cb_Feedback);
	  
	  Bypass = new Fl_Button(xoff+5, yoff+25, 40, 20, "Bypass");
	  Bypass->color(GUIBG2_COLOUR);
	  Bypass->labelsize(10);
	  Bypass->type(1);
	  Bypass->value(1);
	  Bypass->callback((Fl_Callback*)cb_Bypass);
	  
      o->end();
    
}

void DelayGUI::UpdateValues()
{
	DelayA->value(m_delay->GetDelay());
	Feedback->value(m_delay->GetFeedback());
	Bypass->value((int)m_delay->GetBypass());
}

//// Callbacks ////

inline void DelayGUI::cb_Delay_i(Fl_Knob* o, void* v) 
{ m_delay->SetDelay(o->value()); }
void DelayGUI::cb_Delay(Fl_Knob* o, void* v) 
{ ((DelayGUI*)(o->parent()->user_data()))->cb_Delay_i(o,v); }

inline void DelayGUI::cb_Feedback_i(Fl_Knob* o, void* v) 
{ m_delay->SetFeedback(o->value()); }
void DelayGUI::cb_Feedback(Fl_Knob* o, void* v) 
{ ((DelayGUI*)(o->parent()->user_data()))->cb_Feedback_i(o,v); }

inline void DelayGUI::cb_Bypass_i(Fl_Button* o, void* v) 
{ m_delay->SetBypass(o->value()); }
void DelayGUI::cb_Bypass(Fl_Button* o, void* v) 
{ ((DelayGUI*)(o->parent()->user_data()))->cb_Bypass_i(o,v); }
