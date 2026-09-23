/*  SpiralLoops
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

#include "FilterGUI.h"

static const int GUI_COLOUR = 154;
static const int GUIBG_COLOUR = 144;
static const int GUIBG2_COLOUR = FL_GRAY;


FilterGUI::FilterGUI(Filter *o)
{
	m_filter=o;
	if (!m_filter) cerr<<"WARNING: Filter not correctly set up"<<endl;
}
		
void FilterGUI::CreateGUI(int xoff, int yoff, char *name)
{
	 Fl_Group* o = GUIFilterGroup = new Fl_Group(xoff, yoff, 120, 110, name);
      o->type(1);
	  o->box(FL_UP_BOX);
      o->labeltype(FL_ENGRAVED_LABEL);
      o->align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE);
	  o->user_data((void*)(this));
	  
   	  Cutoff = new Fl_Slider(xoff+15, yoff+20, 20, 70, "Cutoff");
	  Cutoff->type(4);
	  Cutoff->selection_color(GUI_COLOUR);
      Cutoff->labelsize(10);
	  Cutoff->maximum(10000);
      Cutoff->step(0.1);
      Cutoff->value(1000);
      Cutoff->callback((Fl_Callback*)cb_Cutoff);
	 
	  Resonance = new Fl_Knob(xoff+58, yoff+8, 45, 45, "Emphasis");
      Resonance->color(GUI_COLOUR);
	  Resonance->type(Fl_Knob::DOTLIN);
      Resonance->labelsize(10);
      Resonance->maximum(10);
      Resonance->step(0.1);
      Resonance->value(0);   
	  Resonance->callback((Fl_Callback*)cb_Resonance);

	  RevCutoff = new Fl_Button(xoff+57, yoff+72, 50, 15, "RvCMod");
      RevCutoff->type(1);
      RevCutoff->down_box(FL_DOWN_BOX);
      RevCutoff->labelsize(10);
      RevCutoff->callback((Fl_Callback*)cb_RevCutoff);   
	 
	  RevResonance = new Fl_Button(xoff+57, yoff+88, 50, 15, "RvRMod");
      RevResonance->type(1);
      RevResonance->down_box(FL_DOWN_BOX);
      RevResonance->labelsize(10);
      RevResonance->callback((Fl_Callback*)cb_RevResonance);

	  
      o->end();
    
}

void FilterGUI::UpdateValues()
{		 
	Cutoff->value(10000.0f-m_filter->GetCutoff()-10.0f);
	Resonance->value(m_filter->GetResonance()-1.0f);

	RevCutoff->value(0);
	RevResonance->value(0);
	
	if (m_filter->GetRevCutoffMod()) RevCutoff->value(1);
	if (m_filter->GetRevResonanceMod()) RevResonance->value(1);
}

//// Callbacks ////

inline void FilterGUI::cb_Cutoff_i(Fl_Slider* o, void* v) 
{ m_filter->SetCutoff((10000.0f-o->value())+10.0f); }
void FilterGUI::cb_Cutoff(Fl_Slider* o, void* v) 
{ ((FilterGUI*)(o->parent()->user_data()))->cb_Cutoff_i(o,v); }

inline void FilterGUI::cb_Resonance_i(Fl_Knob* o, void* v) 
{ m_filter->SetResonance(o->value()+1.0f); }
void FilterGUI::cb_Resonance(Fl_Knob* o, void* v) 
{ ((FilterGUI*)(o->parent()->user_data()))->cb_Resonance_i(o,v); }

inline void FilterGUI::cb_RevCutoff_i(Fl_Button* o, void* v) 
{ m_filter->SetRevCutoffMod(o->value()); }
void FilterGUI::cb_RevCutoff(Fl_Button* o, void* v) 
{ ((FilterGUI*)(o->parent()->user_data()))->cb_RevCutoff_i(o,v); }

inline void FilterGUI::cb_RevResonance_i(Fl_Button* o, void* v) 
{ m_filter->SetRevResonanceMod(o->value()); }
void FilterGUI::cb_RevResonance(Fl_Button* o, void* v) 
{ ((FilterGUI*)(o->parent()->user_data()))->cb_RevResonance_i(o,v); }

