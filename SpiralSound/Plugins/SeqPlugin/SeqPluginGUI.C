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

#include <stdio.h>
#include "SeqPluginGUI.h"
#include <FL/fl_draw.H>
#include <FL/fl_draw.H>
#include <FL/fl_file_chooser.H>

using namespace std;

////////////////////////////////////////////

SeqPluginGUI::SeqPluginGUI(int w, int h,SeqPlugin *o,const HostInfo *Info) :
SpiralPluginGUI(w,h,o)
{	
	m_Plugin=o;
	//size_range(10,10);
	
	m_Scroll = new Fl_Scroll(50, 20, w-57, h-26, "");
	add(m_Scroll);
	
	for (int n=0; n<NUM_PATTERNS; n++)
	{
		m_Plugin->SetEventMap(n,new Fl_EventMap(0, 0, 1000, 1000, ""));
		m_Plugin->GetEventMap(n)->SetType(Fl_EventMap::MELODY_MAP);
		m_Scroll->add(m_Plugin->GetEventMap(n));
		m_Plugin->GetEventMap(n)->CreateWindow();
		m_Plugin->GetEventMap(n)->SetUpdateLineClip(0, 18, w, h-38);
		m_Plugin->GetEventMap(n)->hide();
	}
	
	m_Plugin->GetEventMap(0)->show();
	
	m_Pattern = new Fl_Counter(5, 20, 40, 20, "Pattern");
        m_Pattern->labelsize(10);
	m_Pattern->type(FL_SIMPLE_COUNTER);
	m_Pattern->step(1);
	m_Pattern->value(0);
	m_Pattern->callback((Fl_Callback*)cb_Pattern);
	add(m_Pattern);
	
	m_Length = new Fl_Knob(5, 60, 40, 40, "Length");
        m_Length->color(Info->GUI_COLOUR);
	m_Length->type(Fl_Knob::DOTLIN);
        m_Length->labelsize(10);
        m_Length->maximum(30);
        m_Length->step(0.01);
        m_Length->value(1.0);
	m_Length->callback((Fl_Callback*)cb_Length);
	add(m_Length);
	
	m_Speed = new Fl_Knob(5, 115, 40, 40, "Speed");
    m_Speed->color(Info->GUI_COLOUR);
	m_Speed->type(Fl_Knob::DOTLIN);
    m_Speed->labelsize(10);
    m_Speed->maximum(4);
    m_Speed->step(0.01);
    m_Speed->value(2.0);   
	m_Speed->callback((Fl_Callback*)cb_Speed);
	add(m_Speed);
	
	m_Zoom = new Fl_Knob(5,170,40,40,"Zoom");
	m_Zoom->color(Info->GUI_COLOUR);
	m_Zoom->type(Fl_Knob::DOTLIN);
    m_Zoom->labelsize(10);
    m_Zoom->maximum(2);
    m_Zoom->step(0.01);
    m_Zoom->value(1.0);   
	m_Zoom->callback((Fl_Callback*)cb_Zoom);
	add(m_Zoom);
	
	m_Update = new Fl_Button(2,225,45,20,"Update");
	m_Update->type(1);
	m_Update->labelsize(10);
	m_Update->value(1);
	m_Update->callback((Fl_Callback*)cb_Update,NULL);
	add(m_Update);
	
	m_NoteCut = new Fl_Button(2,245,45,20,"NoteCut");
	m_NoteCut->type(1);
	m_NoteCut->labelsize(10);
	m_NoteCut->value(0);
	m_NoteCut->callback((Fl_Callback*)cb_NoteCut);
	add(m_NoteCut);
	
	m_Clear = new Fl_Button(2,265,45,20,"Clear");
	m_Clear->type(0);
	m_Clear->labelsize(10);
	m_Clear->value(0);
	m_Clear->callback((Fl_Callback*)cb_Clear);
	add(m_Clear);
	
	end();
}

void SeqPluginGUI::UpdateValues()
{
}
	
inline void SeqPluginGUI::cb_Update_i(Fl_Button* o, void* v)
{ 
	m_Plugin->SetUpdate(o->value());
}
void SeqPluginGUI::cb_Update(Fl_Button* o, void* v)
{ ((SeqPluginGUI*)(o->parent()))->cb_Update_i(o,v);}

inline void SeqPluginGUI::cb_NoteCut_i(Fl_Button* o, void* v)
{ 
	m_Plugin->SetNoteCut(o->value());
}
void SeqPluginGUI::cb_NoteCut(Fl_Button* o, void* v)
{ ((SeqPluginGUI*)(o->parent()))->cb_NoteCut_i(o,v);}

inline void SeqPluginGUI::cb_Zoom_i(Fl_Knob* o, void* v)
{
	m_Plugin->SetZoom(o->value()); 
}
void SeqPluginGUI::cb_Zoom(Fl_Knob* o, void* v)
{ ((SeqPluginGUI*)(o->parent()))->cb_Zoom_i(o,v);}



inline void SeqPluginGUI::cb_Pattern_i(Fl_Counter* o, void* v)
{ 	
	if (o->value()<0) o->value(0);
	if (o->value()>NUM_PATTERNS-1) o->value(NUM_PATTERNS-1);
	m_Plugin->SetPattern((int)o->value());
	//UpdateValues();
}
void SeqPluginGUI::cb_Pattern(Fl_Counter* o, void* v)
{ ((SeqPluginGUI*)(o->parent()))->cb_Pattern_i(o,v);}

inline void SeqPluginGUI::cb_Length_i(Fl_Knob* o, void* v)
{
	m_Plugin->SetEndTime(o->value());
}
void SeqPluginGUI::cb_Length(Fl_Knob* o, void* v)
{ ((SeqPluginGUI*)(o->parent()))->cb_Length_i(o,v);}

inline void SeqPluginGUI::cb_Speed_i(Fl_Knob* o, void* v)
{ 	
	m_Plugin->SetSpeed(o->value()-2.0f);
}
void SeqPluginGUI::cb_Speed(Fl_Knob* o, void* v)
{ ((SeqPluginGUI*)(o->parent()))->cb_Speed_i(o,v);}

inline void SeqPluginGUI::cb_Clear_i(Fl_Button* o, void* v)
{ 	
	m_Plugin->ClearAll();
}
void SeqPluginGUI::cb_Clear(Fl_Button* o, void* v)
{ ((SeqPluginGUI*)(o->parent()))->cb_Clear_i(o,v);}

