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
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Output.H>

#include "SeqPlugin.h"
#include "SpiralPluginGUI.h"
#include "Fl_Knob.H"
#include "Fl_EventMap.h"

#ifndef MixerGUI
#define MixerGUI


class SeqPluginGUI : public SpiralPluginGUI
{
public:
	SeqPluginGUI(int w, int h, SeqPlugin *o,const HostInfo *Info);
	
	virtual void UpdateValues();
	virtual SpiralPlugin* GetPlugin() { return m_Plugin; }
	
	SeqPlugin *m_Plugin;	
private:
	
	Fl_Scroll*  m_Scroll;
	Fl_Button*  m_Update;
	Fl_Button*  m_NoteCut;
	Fl_Knob*    m_Zoom;
	Fl_Counter* m_Pattern;
	Fl_Knob*    m_Length;
	Fl_Knob*    m_Speed;
	Fl_Button*  m_Clear;
	
	//// Callbacks ////
	inline void cb_Update_i(Fl_Button* o, void* v);
	static void cb_Update(Fl_Button* o, void* v); 
	inline void cb_Zoom_i(Fl_Knob* o, void* v);
	static void cb_Zoom(Fl_Knob* o, void* v); 
	inline void cb_NoteCut_i(Fl_Button* o, void* v);
	static void cb_NoteCut(Fl_Button* o, void* v);
	inline void cb_Pattern_i(Fl_Counter* o, void* v);
	static void cb_Pattern(Fl_Counter* o, void* v); 
	inline void cb_Length_i(Fl_Knob* o, void* v);
	static void cb_Length(Fl_Knob* o, void* v); 
	inline void cb_Speed_i(Fl_Knob* o, void* v);
	static void cb_Speed(Fl_Knob* o, void* v); 
	inline void cb_Clear_i(Fl_Button* o, void* v);
	static void cb_Clear(Fl_Button* o, void* v); 
};

#endif
