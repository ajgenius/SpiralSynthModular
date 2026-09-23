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
#include <FL/Fl_Input.H>

#include "SequencerPlugin.h"
#include "SpiralPluginGUI.h"
#include "Fl_Knob.H"
#include "Fl_EventMap.h"

#ifndef MixerGUI
#define MixerGUI


class SequencerPluginGUI : public SpiralPluginGUI
{
public:
	SequencerPluginGUI(int w, int h, SequencerPlugin *o, ChannelHandler *ch,const HostInfo *Info);
	
	virtual void UpdateValues(SpiralPlugin *o);
	virtual void Update();

private:
	
	Fl_Scroll*  m_Scroll;
	Fl_Button*  m_NewPattern;
	Fl_Button*  m_NoteCut;
	Fl_Knob*    m_Zoom;
	Fl_Knob*    m_Speed;
	Fl_Button*  m_Clear;
	Fl_Input*   m_Length;
	Fl_Input*   m_BeatsPerBar;
	Fl_Knob*    m_BarLength;
	
	int m_NextPatternID;
	
	class PatternWin : public Fl_Double_Window
	{
	public:
		PatternWin(int w,int h,const char* n);
		Fl_EventMap *GetEventMap() { return m_Melody; }
		
	private:
		Fl_Scroll*  m_Scroll;
		Fl_EventMap* m_Melody;
		int m_NextNoteID;
	};
	
	map<int,PatternWin*> m_PatternWinMap;
	Fl_EventMap* m_ArrangementMap;
	
	// get the notes from the audio thread
	void LoadPatternData(int ID);
	
	void ChangeSequenceHelper(Fl_SEvent *event);

	//// Callbacks ////
	inline void cb_Zoom_i(Fl_Knob* o, void* v);
	static void cb_Zoom(Fl_Knob* o, void* v); 
	inline void cb_NoteCut_i(Fl_Button* o, void* v);
	static void cb_NoteCut(Fl_Button* o, void* v);
	inline void cb_Pattern_i(Fl_Counter* o, void* v);
	static void cb_Pattern(Fl_Counter* o, void* v); 
	inline void cb_Length_i(Fl_Input* o, void* v);
	static void cb_Length(Fl_Input* o, void* v); 
	inline void cb_BeatsPerBar_i(Fl_Input* o, void* v);
	static void cb_BeatsPerBar(Fl_Input* o, void* v); 
	inline void cb_BarLength_i(Fl_Knob* o, void* v);
	static void cb_BarLength(Fl_Knob* o, void* v); 
	inline void cb_Speed_i(Fl_Knob* o, void* v);
	static void cb_Speed(Fl_Knob* o, void* v); 
	inline void cb_Clear_i(Fl_Button* o, void* v);
	static void cb_Clear(Fl_Button* o, void* v); 
	inline void cb_NewPattern_i(Fl_Button* o, void* v);
	static void cb_NewPattern(Fl_Button* o, void* v); 
	
	// sequence event callbacks
	inline void cb_ArrangeRM_i(Fl_Button* o, void* v);
	static void cb_ArrangeRM(Fl_Button* o, void* v); 
	inline void cb_MoveSequence_i(Fl_Widget* o, void* v);
	static void cb_MoveSequence(Fl_Widget* o, void* v); 
	inline void cb_Rename_i(Fl_Widget* o, void* v);
	static void cb_Rename(Fl_Widget* o, void* v); 
	inline void cb_Recolour_i(Fl_Widget* o, void* v);
	static void cb_Recolour(Fl_Widget* o, void* v); 
	inline void cb_Copy_i(Fl_Widget* o, void* v);
	static void cb_Copy(Fl_Widget* o, void* v); 
	inline void cb_RemoveSequence_i(Fl_Widget* o, void* v);
	static void cb_RemoveSequence(Fl_Widget* o, void* v); 
	inline void cb_Edit_i(Fl_Widget* o, void* v);
	static void cb_Edit(Fl_Widget* o, void* v); 
	
	// note event callbacks
	inline void cb_NewNote_i(Fl_Widget* o, void* v);
	static void cb_NewNote(Fl_Widget* o, void* v); 
	inline void cb_MoveNote_i(Fl_Widget* o, void* v);
	static void cb_MoveNote(Fl_Widget* o, void* v); 
	inline void cb_RemoveNote_i(Fl_Widget* o, void* v);
	static void cb_RemoveNote(Fl_Widget* o, void* v); 
};

#endif
