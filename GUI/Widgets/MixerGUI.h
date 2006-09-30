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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Check_Button.H>
#include "Widgets/Fl_Knob.H"

#include "GUIBase.h"

#ifndef MIXGUI
#define MIXGUI

class MixerGUI : public GUIBase
{
public:
	MixerGUI(Mixer *o);
	
	virtual void CreateGUI(int xoff=0, int yoff=0, char *name="");	
	virtual void UpdateValues();
	
	Mixer *m_mix;	
	
private:

	Fl_Group 		*GUIMixGroup;
	Fl_Check_Button *Add;
	Fl_Check_Button *XMod;
	Fl_Check_Button *Ring;
		
	Fl_Knob 		*Balance;
	
	//// Callbacks ////
	
	inline void cb_Balance_i(Fl_Knob* o, void* v);
    static void cb_Balance(Fl_Knob*, void*);
	inline void cb_XModAmount_i(Fl_Knob* o, void* v);
	static void cb_XModAmount(Fl_Knob* o, void* v);
	inline void cb_Add_i(Fl_Check_Button* o, void* v);
    static void cb_Add(Fl_Check_Button*, void*);
	inline void cb_Mult_i(Fl_Check_Button* o, void* v);
    static void cb_Mult(Fl_Check_Button*, void*);
	inline void cb_Sub_i(Fl_Check_Button* o, void* v);
	static void cb_Sub(Fl_Check_Button* o, void* v); 
	inline void cb_XMod_i(Fl_Check_Button* o, void* v);
	static void cb_XMod(Fl_Check_Button* o, void* v); 
	inline void cb_Ring_i(Fl_Check_Button* o, void* v);
	static void cb_Ring(Fl_Check_Button* o, void* v); 

  
};

#endif
