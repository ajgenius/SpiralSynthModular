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

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Value_Output.H>
#include "Widgets/Fl_Knob.H"
#include "Widgets/Fl_Loop.h"
#include "Widgets/Fl_LED_Button.H"
#include "Widgets/Fl_Trigger.h"

#include "GUIBase.h"
#include "../SpiralSound/Filter.h"
#include "../SpiralSound/Delay.h"
#include "../SpiralSound/Reverb.h"
#include <vector>

#ifndef LOOPGUI
#define LOOPGUI

class Loop;

class LoopGUI : public GUIBase
{
public:
	LoopGUI(Loop *o);
	~LoopGUI();	

	virtual void CreateGUI(char *name="");	
	virtual void UpdateValues();	
	
	int  GetPos();
	void SetMasterStatus(bool m);
	void RefreshWave() {}
	void CheckTriggers(float CurrentAngle, vector<int> &LoopTriggers);
	void StopUpdate(bool s) {GUILoopGroup->StopUpdate(s);}
	void Trigger() {Play->value(true);}
	void UpdateDataPtr();
	void SetSnap(bool s);
	void SetSnapAngle(int Angle);
	void UpdatePos() { GUILoopGroup->DrawPosMarker(); }
	
	Fl_Group *GetGUI() { return GUILoopGroup; }
	
private:
	Loop *m_loop;
	
	float m_LastAngle;
	int   m_SnapAngle;
	int   m_TriggerCount;
	
	Fl_Loop 		 *GUILoopGroup;	
	Fl_Group	     *InnerGroup;
	Fl_Slider 		 *Cutoff;
	Fl_Slider 		 *Volume;
	Fl_Knob 		 *Balance;
	Fl_Knob 		 *Length;
	Fl_Knob 		 *Speed;
	Fl_Knob 		 *Resonance;
	Fl_Value_Output  *SpeedOutput;
	Fl_Value_Output  *LengthOutput;
	Fl_Value_Output  *MagicOutput;
	Fl_Knob 		 *WaveSize;
	
	Fl_Button		 *Record;
	Fl_Button		 *Load;
	Fl_LED_Button	 *Play;
	Fl_LED_Button	 *Master;	
	Fl_LED_Button	 *Update;	
	Fl_Button		 *Effects;	
	Fl_Button		 *Double;	
	Fl_Button		 *Match;	
	Fl_Button		 *Save;	
	
	vector<Fl_Trigger*> m_TriggerVec;
	
	//// Callbacks ////
	inline void cb_Loop_i(Fl_Loop* o, void* v);
	static void cb_Loop(Fl_Loop* o, void* v);
	inline void cb_Volume_i(Fl_Slider* o, void* v);
	static void cb_Volume(Fl_Slider* o, void* v);
	inline void cb_Cutoff_i(Fl_Slider* o, void* v);
	static void cb_Cutoff(Fl_Slider* o, void* v);
	inline void cb_Balance_i(Fl_Knob* o, void* v);
	static void cb_Balance(Fl_Knob* o, void* v);
	inline void cb_Length_i(Fl_Knob* o, void* v);
	static void cb_Length(Fl_Knob* o, void* v);
	inline void cb_Speed_i(Fl_Knob* o, void* v);
	static void cb_Speed(Fl_Knob* o, void* v);
	inline void cb_Resonance_i(Fl_Knob* o, void* v);
	static void cb_Resonance(Fl_Knob* o, void* v);
	inline void cb_Record_i(Fl_Button* o, void* v);
	static void cb_Record(Fl_Button* o, void* v);
	inline void cb_Load_i(Fl_Button* o, void* v);
	static void cb_Load(Fl_Button* o, void* v);
	inline void cb_Play_i(Fl_LED_Button* o, void* v);
	static void cb_Play(Fl_LED_Button* o, void* v);
	inline void cb_Master_i(Fl_LED_Button* o, void* v);
	static void cb_Master(Fl_LED_Button* o, void* v);
	inline void cb_Effects_i(Fl_Button* o, void* v);
	static void cb_Effects(Fl_Button* o, void* v);
	inline void cb_Update_i(Fl_LED_Button* o, void* v);
	static void cb_Update(Fl_LED_Button* o, void* v);
	inline void cb_Double_i(Fl_Button* o, void* v);
	static void cb_Double(Fl_Button* o, void* v);
	inline void cb_Match_i(Fl_Button* o, void* v);
	static void cb_Match(Fl_Button* o, void* v);
	inline void cb_Save_i(Fl_Button* o, void* v);
	static void cb_Save(Fl_Button* o, void* v);
	inline void cb_WaveSize_i(Fl_Knob* o, void* v);
	static void cb_WaveSize(Fl_Knob* o, void* v);
	
	inline void cb_Cut_i(Fl_Loop *o, int Start, int End);
	static void cb_Cut(Fl_Loop *o, int Start, int End);
	inline void cb_Copy_i(Fl_Loop *o, int Start, int End);
	static void cb_Copy(Fl_Loop *o, int Start, int End);
	inline void cb_Paste_i(Fl_Loop *o, int Start);
	static void cb_Paste(Fl_Loop *o, int Start);
	inline void cb_PasteMix_i(Fl_Loop *o, int Start);
	static void cb_PasteMix(Fl_Loop *o, int Start);
	inline void cb_ZeroRange_i(Fl_Loop *o, int Start, int End);
	static void cb_ZeroRange(Fl_Loop *o, int Start, int End);
	inline void cb_ReverseRange_i(Fl_Loop *o, int Start, int End);
	static void cb_ReverseRange(Fl_Loop *o, int Start, int End);
	inline void cb_Halve_i(Fl_Loop *o, int Start, int End);
	static void cb_Halve(Fl_Loop *o, int Start, int End);
	inline void cb_Hold_i(Fl_Loop *o, int Start, int End);
	static void cb_Hold(Fl_Loop *o, int Start, int End);
	inline void cb_SelectAll_i(Fl_Loop *o, int Start, int End);
	static void cb_SelectAll(Fl_Loop *o, int Start, int End);
	inline void cb_NewTrigger_i(Fl_Loop *o, int Start, int End);
	static void cb_NewTrigger(Fl_Loop *o, int Start, int End);
	inline void cb_Move_i(Fl_Loop *o, int Start, int End);
	static void cb_Move(Fl_Loop *o, int Start, int End);
	
	friend istream &operator>>(istream &s, LoopGUI &o);
	friend ostream &operator<<(ostream &s, LoopGUI &o);	
};

istream &operator>>(istream &s, LoopGUI &o);
ostream &operator<<(ostream &s, LoopGUI &o);

#endif
