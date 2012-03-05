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

#include "SamplerPluginGUI.h"
#include <FL/fl_draw.H>
#include <FL/fl_draw.H>
#include <FL/fl_file_chooser.H>

using namespace std;

////////////////////////////////////////////

SamplerPluginGUI::SamplerPluginGUI(int w, int h,SamplerPlugin *o,const HostInfo *Info) :
SpiralPluginGUI(w,h,o)
{	
	m_Plugin=o;
		
	for (int n=0; n<NUM_SAMPLES; n++)
	{
		Numbers[n]=n;
		
		m_Load[n] = new Fl_Button(5, 20+n*50, 50, 30, "Load");
    	m_Load[n]->labelsize(10);
		m_Load[n]->callback((Fl_Callback*)cb_Load,(void*)&Numbers[n]);
		add(m_Load[n]);
	
		m_Volume[n] = new Fl_Knob(60, 15+n*50, 38, 38, "Volume");
    	m_Volume[n]->color(Info->GUI_COLOUR);
		m_Volume[n]->type(Fl_Knob::LINELIN);
    	m_Volume[n]->labelsize(10);
    	m_Volume[n]->maximum(2);
    	m_Volume[n]->step(0.001);
    	m_Volume[n]->value(1);   
		m_Volume[n]->callback((Fl_Callback*)cb_Volume,(void*)&Numbers[n]);
		add(m_Volume[n]);
	
		m_Pitch[n] = new Fl_Knob(100, 15+n*50, 38, 38, "Pitch");
    	m_Pitch[n]->color(Info->GUI_COLOUR);
		m_Pitch[n]->type(Fl_Knob::LINELIN);
    	m_Pitch[n]->labelsize(10);
    	m_Pitch[n]->maximum(10);
    	m_Pitch[n]->step(0.001);
    	m_Pitch[n]->value(1);   
		m_Pitch[n]->callback((Fl_Callback*)cb_Pitch,(void*)&Numbers[n]);
		add(m_Pitch[n]);
	
		m_Loop[n] = new Fl_Button(140, 20+n*50, 50, 30, "Loop");
    	m_Loop[n]->labelsize(10);
		m_Loop[n]->type(1);
		m_Loop[n]->callback((Fl_Callback*)cb_Loop,(void*)&Numbers[n]);
		add(m_Loop[n]);
	
		m_Note[n] = new Fl_Counter(190, 20+n*50, 50, 30, "Trig Note");
    	m_Note[n]->labelsize(10);
		m_Note[n]->type(FL_SIMPLE_COUNTER);
		m_Note[n]->step(1);
		m_Note[n]->value(n);
		m_Note[n]->callback((Fl_Callback*)cb_Note,(void*)&Numbers[n]);
		add(m_Note[n]);
	}
	
	end();
}

void SamplerPluginGUI::UpdateValues()
{
	for (int n=0; n<NUM_SAMPLES; n++)
	{
		m_Volume[n]->value(m_Plugin->GetVolume(n));
		m_Pitch[n]->value(m_Plugin->GetPitch(n));
		m_Note[n]->value(m_Plugin->GetNote(n));	
		m_Loop[n]->value(m_Plugin->GetLoop(n));
	}
}
	
inline void SamplerPluginGUI::cb_Load_i(Fl_Button* o, void* v)
{ 
	char *fn=fl_file_chooser("Load a sample", "{*.wav,*.WAV}",NULL);
		
	if (fn && fn!='\0')
	{
		m_Plugin->LoadSample(*(int*)v,fn); 	
	}
}
void SamplerPluginGUI::cb_Load(Fl_Button* o, void* v)
{ ((SamplerPluginGUI*)(o->parent()))->cb_Load_i(o,v);}

inline void SamplerPluginGUI::cb_Volume_i(Fl_Knob* o, void* v)
{ m_Plugin->SetVolume(*(int*)v,o->value()); }
void SamplerPluginGUI::cb_Volume(Fl_Knob* o, void* v)
{ ((SamplerPluginGUI*)(o->parent()))->cb_Volume_i(o,v);}

inline void SamplerPluginGUI::cb_Pitch_i(Fl_Knob* o, void* v)
{ m_Plugin->SetPitch(*(int*)v,o->value()); }
void SamplerPluginGUI::cb_Pitch(Fl_Knob* o, void* v)
{ ((SamplerPluginGUI*)(o->parent()))->cb_Pitch_i(o,v);}

inline void SamplerPluginGUI::cb_Loop_i(Fl_Button* o, void* v)
{ m_Plugin->SetLoop(*(int*)v,o->value()); }
void SamplerPluginGUI::cb_Loop(Fl_Button* o, void* v)
{ ((SamplerPluginGUI*)(o->parent()))->cb_Loop_i(o,v);}

inline void SamplerPluginGUI::cb_Note_i(Fl_Counter* o, void* v)
{ m_Plugin->SetNote(*(int*)v,(int)o->value()); }
void SamplerPluginGUI::cb_Note(Fl_Counter* o, void* v)
{ ((SamplerPluginGUI*)(o->parent()))->cb_Note_i(o,v);}

