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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Counter.H>

#include "SamplerPlugin.h"
#include "SpiralPluginGUI.h"
#include "Fl_Knob.H"

#ifndef MixerGUI
#define MixerGUI


class SamplerPluginGUI : public SpiralPluginGUI
{
public:
	SamplerPluginGUI(int w, int h, SamplerPlugin *o,const HostInfo *Info);
	
	virtual void UpdateValues();
	virtual SpiralPlugin* GetPlugin() { return m_Plugin; }
	
	SamplerPlugin *m_Plugin;	
private:
	
	int Numbers[NUM_SAMPLES];
	
	Fl_Button* m_Load[NUM_SAMPLES];
	Fl_Knob* m_Volume[NUM_SAMPLES];
	Fl_Knob* m_Pitch[NUM_SAMPLES];
	Fl_Button* m_Loop[NUM_SAMPLES];
	Fl_Counter* m_Note[NUM_SAMPLES];
	
	//// Callbacks ////
	inline void cb_Load_i(Fl_Button* o, void* v);
	static void cb_Load(Fl_Button* o, void* v); 
	inline void cb_Volume_i(Fl_Knob* o, void* v);
	static void cb_Volume(Fl_Knob* o, void* v); 
	inline void cb_Pitch_i(Fl_Knob* o, void* v);
	static void cb_Pitch(Fl_Knob* o, void* v); 
	inline void cb_Loop_i(Fl_Button* o, void* v);
	static void cb_Loop(Fl_Button* o, void* v); 
	inline void cb_Note_i(Fl_Counter* o, void* v);
	static void cb_Note(Fl_Counter* o, void* v); 

};

#endif
