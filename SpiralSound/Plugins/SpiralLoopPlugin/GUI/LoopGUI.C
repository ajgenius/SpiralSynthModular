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
#include <iostream>

#include "Widgets/Fl_Loop.h"
#include "LoopGUI.h"
#include "../Loop.h"
#include <FL/fl_file_chooser.H>

static const int EFFECT_ADDITION = 130;
static const int GUI_COLOUR = 154;
static const int GUIBG_COLOUR = FL_GRAY;
static const int GUIBG2_COLOUR = FL_GRAY;

static const int MAX_TRIGGERS = 8;

//static const int GUIBG_COLOUR = 144;
//static const int GUIBG2_COLOUR = 145;

/////////////////////////////////////////////////////////////////////////////

LoopGUI::LoopGUI(Loop *o) :
m_TriggerCount(0),
GUILoopGroup(NULL)
{
	m_loop=o;
	m_LastAngle=0;	
	
	if (!m_loop) cerr<<"WARNING: Loop not correctly set up"<<endl;
}
	
LoopGUI::~LoopGUI() 
{	
	delete GUILoopGroup;	
}

void LoopGUI::CreateGUI(char *name)
{
	int y=20;
	{
	  GUILoopGroup = new Fl_Loop(0,y,300, 300, name);
      GUILoopGroup->box(FL_NO_BOX);
	  GUILoopGroup->color(GUIBG2_COLOUR);
	  GUILoopGroup->align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE);
	  GUILoopGroup->user_data((void*)(this));
	  GUILoopGroup->SetData(m_loop->GetLoopPtr());
	  GUILoopGroup->SetLength(m_loop->GetLoopLength());
	  GUILoopGroup->SetPos(m_loop->GetPosPtr());
	  GUILoopGroup->callback((Fl_Callback*)cb_Loop);
	  GUILoopGroup->SetupCopyBufFuncs((Fl_Loop::cb_CopyBuf1*)cb_Cut, 
	                                  (Fl_Loop::cb_CopyBuf1*)cb_Copy, 
	                                  (Fl_Loop::cb_CopyBuf2*)cb_Paste,
									  (Fl_Loop::cb_CopyBuf2*)cb_PasteMix,
									  (Fl_Loop::cb_CopyBuf1*)cb_ZeroRange,
					    			  (Fl_Loop::cb_CopyBuf1*)cb_ReverseRange,
					    			  (Fl_Loop::cb_CopyBuf1*)cb_Halve,
					    			  (Fl_Loop::cb_CopyBuf1*)cb_Hold,
					    			  (Fl_Loop::cb_CopyBuf1*)cb_SelectAll,
									  (Fl_Loop::cb_CopyBuf1*)cb_NewTrigger,
									  (Fl_Loop::cb_CopyBuf1*)cb_Move);
							  
	  //GUILoopGroup->SetMainWin(m_loop->GetParent()->GetPluginWindow());
	   
	  Volume = new Fl_Slider(60, y+100, 20, 70, "Volume");
	  Volume->type(4);
	  Volume->color(GUIBG2_COLOUR);
	  Volume->selection_color(GUI_COLOUR);
      Volume->labelsize(10);
	  Volume->maximum(1.0f);
      Volume->step(0.001);
      Volume->value(0);
      Volume->callback((Fl_Callback*)cb_Volume);
	  GUILoopGroup->add(Volume);
	  
	  Cutoff = new Fl_Slider(220, y+100, 20, 70, "Cutoff");
	  Cutoff->type(4);
	  Cutoff->color(GUIBG2_COLOUR);
	  Cutoff->selection_color(GUI_COLOUR);
      Cutoff->labelsize(10);
	  Cutoff->maximum(100);
      Cutoff->step(0.1);
      Cutoff->value(0);
      Cutoff->callback((Fl_Callback*)cb_Cutoff);
      GUILoopGroup->add(Cutoff);
		  	 
	  Resonance = new Fl_Knob(160, y+140, 45, 45, "Emphasis");
      Resonance->color(GUI_COLOUR);
	  Resonance->type(Fl_Knob::DOTLIN);
      Resonance->labelsize(10);
      Resonance->maximum(10);
      Resonance->step(0.1);
      Resonance->value(0);   
	  Resonance->callback((Fl_Callback*)cb_Resonance);
      GUILoopGroup->add(Resonance);
	  
 	  Balance = new Fl_Knob(95, y+140, 45, 45, "Balance");
      Balance->color(GUI_COLOUR);
	  Balance->type(Fl_Knob::DOTLIN);
      Balance->labelsize(10);
      Balance->maximum(2);
      Balance->step(0.001);
      Balance->value(1.0);   
	  Balance->callback((Fl_Callback*)cb_Balance);
	  GUILoopGroup->add(Balance);
	  
 	  Speed = new Fl_Knob(95, y+60, 45, 45, "Speed");
      Speed->color(GUI_COLOUR);
	  Speed->type(Fl_Knob::DOTLIN);
      Speed->labelsize(10);
      Speed->maximum(3);
      Speed->step(0.0001);
      Speed->value(1);   
	  Speed->callback((Fl_Callback*)cb_Speed);
	  GUILoopGroup->add(Speed);
	  
	  Length = new Fl_Knob(160, y+60, 45, 45, "Length");
      Length->color(GUI_COLOUR);
	  Length->type(Fl_Knob::DOTLIN);
      Length->labelsize(10);
      Length->maximum(1);
      Length->step(0.0001);
      Length->value(1);   
	  Length->callback((Fl_Callback*)cb_Length);
	  GUILoopGroup->add(Length);

	  Record = new Fl_Button(85, y+200, 60, 14, "Record");
      Record->type(1);
	  Record->color(GUIBG2_COLOUR);
      Record->down_box(FL_DOWN_BOX);
      Record->labelsize(10);
      Record->callback((Fl_Callback*)cb_Record); 
	  GUILoopGroup->add(Record);
	  
	  Load = new Fl_Button(155, y+200, 60, 14, "Load");
	  Load->color(GUIBG2_COLOUR);
      Load->type(0);
      Load->down_box(FL_DOWN_BOX);
      Load->labelsize(10);
      Load->callback((Fl_Callback*)cb_Load); 
	  GUILoopGroup->add(Load);
	  
	  //Effects = new Fl_Button(130, y+200, 40, 14, "Effects");
	  //Effects->color(GUIBG2_COLOUR);
      //Effects->type(1);
      //Effects->down_box(FL_DOWN_BOX);
      //Effects->labelsize(10);
      //Effects->callback((Fl_Callback*)cb_Effects);
	  
	  Double = new Fl_Button(85, y+218, 60, 14, "Double");
	  Double->color(GUIBG2_COLOUR);
      Double->type(0);
      Double->down_box(FL_DOWN_BOX);
      Double->labelsize(10);
      Double->callback((Fl_Callback*)cb_Double);
	  GUILoopGroup->add(Double);
	  
	  //Match = new Fl_Button(130, y+218, 40, 14, "Match");
	  //Match->color(GUIBG2_COLOUR);
      //Match->type(0);
      //Match->down_box(FL_DOWN_BOX);
      //Match->labelsize(10);
      //Match->callback((Fl_Callback*)cb_Match);
	  
	  Save = new Fl_Button(155, y+218, 60, 14, "Save");
	  Save->color(GUIBG2_COLOUR);
      Save->type(0);
      Save->down_box(FL_DOWN_BOX);
      Save->labelsize(10);
      Save->callback((Fl_Callback*)cb_Save);
	  GUILoopGroup->add(Save);
	  
	  Play = new Fl_LED_Button(255, y+275, 40, 25, "Play");
	  Play->color(GUIBG2_COLOUR);
      Play->type(1);
	  Play->value(1);
      Play->down_box(FL_DOWN_BOX);
      Play->labelsize(10);
      Play->callback((Fl_Callback*)cb_Play); 
	  GUILoopGroup->add(Play);
	  
	  SpeedOutput  = new Fl_Value_Output(93, y+120, 50, 15);
	  SpeedOutput->labelsize(10);
	  SpeedOutput->textsize(10);
	  GUILoopGroup->add(SpeedOutput);
	  
	  LengthOutput = new Fl_Value_Output(157, y+120, 50, 15);
	  LengthOutput->labelsize(10);
	  LengthOutput->textsize(10);
	  GUILoopGroup->add(LengthOutput);
	  
	  MagicOutput = new Fl_Value_Output(125, y+235, 50, 15);
	  MagicOutput->labelsize(10);
	  MagicOutput->textsize(10);
	  GUILoopGroup->add(MagicOutput);
	  
	  //Master = new Fl_LED_Button(2, y+5, 40, 25, "Master");
	  //Master->type(1);
	  //Master->color(GUIBG2_COLOUR);
      //Master->down_box(FL_DOWN_BOX);
      //Master->labelsize(10);
      //Master->callback((Fl_Callback*)cb_Master); 
	  
	  Update = new Fl_LED_Button(2, y+275, 40, 25, "Update");
	  Update->type(1);
	  Update->value(0);
	  Update->selection_color(3);
      Update->down_box(FL_DOWN_BOX);
      Update->labelsize(10);
      Update->callback((Fl_Callback*)cb_Update); 
	  GUILoopGroup->add(Update);
	  
	  WaveSize = new Fl_Knob(262, y+5, 30, 30, "WaveSize");
      WaveSize->color(GUI_COLOUR);
	  WaveSize->type(Fl_Knob::DOTLIN);
      WaveSize->labelsize(8);
      WaveSize->maximum(10.0);
      WaveSize->step(0.01);
      WaveSize->value(0.0);   
	  WaveSize->callback((Fl_Callback*)cb_WaveSize);
	  GUILoopGroup->add(WaveSize);
	   
      GUILoopGroup->show(); 
	  GUILoopGroup->end();
  }	  
}

void LoopGUI::SetMasterStatus(bool m)
{
	m_loop->SetMasterStatus(m);
	Master->value(m);
	GUILoopGroup->redraw();
}

void LoopGUI::CheckTriggers(float CurrentAngle, vector<int> &LoopTriggers)
{	
	for (vector<Fl_Trigger*>::iterator i=m_TriggerVec.begin();
		 i!=m_TriggerVec.end(); i++)
	{
		// Todo: Better detection needed
		
		if (CurrentAngle>(*i)->GetAngle() && 
		    m_LastAngle<(*i)->GetAngle())
		{
			if ((*i)->Completed() && (*i)->GetPass()!=0)
			{				
				m_loop->SetPlaying(false);	
				Play->value(false);
			}
			
			LoopTriggers.push_back((*i)->GetID());
			(*i)->IncCount();			
		}
	}
	
	m_LastAngle=CurrentAngle;
}

void LoopGUI::SetSnapAngle(int Angle)
{
	m_SnapAngle = Angle;
	
	for (vector<Fl_Trigger*>::iterator i=m_TriggerVec.begin();
		 i!=m_TriggerVec.end(); i++)
	{
		(*i)->SetSnapAngle(Angle);
	}
	
	GUILoopGroup->SetSnapAngle(Angle);
	GUILoopGroup->redraw();
}


void LoopGUI::SetSnap(bool s)
{
	for (vector<Fl_Trigger*>::iterator i=m_TriggerVec.begin();
		 i!=m_TriggerVec.end(); i++)
	{
		(*i)->SetSnap(s);
	}
	
	GUILoopGroup->SetSnap(s);
}

void LoopGUI::UpdateValues()
{
      Volume->value(1.0f-m_loop->GetVolume());
      Cutoff->value(100.0f-sqrt(m_loop->GetCutoff()-10.0f));  
      Resonance->value(m_loop->GetResonance()-1.0f);   	 
      Balance->value(m_loop->GetBalance());   	  
      Speed->value(m_loop->GetSpeed());   	  
      Length->value(m_loop->GetLoopLength()/(float)m_loop->GetTotalLength());
      Effects->value(m_loop->GetEffects());     
	  Play->value(m_loop->IsPlaying());
      Master->value(m_loop->IsMaster());
}

void LoopGUI::UpdateDataPtr() 
{
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
}
	

//// Callbacks ////

inline void LoopGUI::cb_Loop_i(Fl_Loop* o, void* v)
{
	o->hide();
	m_loop->DelMe();	
}
void LoopGUI::cb_Loop(Fl_Loop* o, void* v)
{ ((LoopGUI*)(o->user_data()))->cb_Loop_i(o,v); }
	
inline void LoopGUI::cb_Volume_i(Fl_Slider* o, void* v)
{ 
	m_loop->SetVolume(1.0f-o->value()); 
}
void LoopGUI::cb_Volume(Fl_Slider* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Volume_i(o,v); }

inline void LoopGUI::cb_Cutoff_i(Fl_Slider* o, void* v)
{ 
	m_loop->FilterBypass(o->value()==0);	
	float value=100.0f-o->value();
	m_loop->SetCutoff((int)(value*value+10.0f)); 
}
void LoopGUI::cb_Cutoff(Fl_Slider* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Cutoff_i(o,v); }

inline void LoopGUI::cb_Balance_i(Fl_Knob* o, void* v)
{
	/*float rvalue=1+o->value()-1.0f; 
	float lvalue=2-o->value();
	 
	m_loop->SetLeftVol(lvalue/2.0f);
	m_loop->SetRightVol(rvalue/2.0f);*/
	
	m_loop->SetBalance(o->value());
	
}
void LoopGUI::cb_Balance(Fl_Knob* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Balance_i(o,v); }

inline void LoopGUI::cb_Length_i(Fl_Knob* o, void* v)
{ 
	m_loop->SetLength((int)(o->value()*m_loop->GetTotalLength())); 
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	
	//LengthOutput->value((float)m_loop->GetLoopLength()/(float)SpiralLoopsInfo::SAMPLERATE);
	GUILoopGroup->redraw();
}
void LoopGUI::cb_Length(Fl_Knob* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Length_i(o,v); }

inline void LoopGUI::cb_Speed_i(Fl_Knob* o, void* v)
{ 
	m_loop->SetSpeed(o->value()); 
	SpeedOutput->value(o->value());
}
void LoopGUI::cb_Speed(Fl_Knob* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Speed_i(o,v); }

inline void LoopGUI::cb_Resonance_i(Fl_Knob* o, void* v)
{ 
	m_loop->SetResonance((int)(o->value()+1.0f)); 
}
void LoopGUI::cb_Resonance(Fl_Knob* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Resonance_i(o,v); }

inline void LoopGUI::cb_Record_i(Fl_Button* o, void* v)
{
	m_loop->Record(o->value()); 
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	GUILoopGroup->redraw();
}
void LoopGUI::cb_Record(Fl_Button* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Record_i(o,v); }

inline void LoopGUI::cb_Load_i(Fl_Button* o, void* v)
{ 
	//GUILoopGroup->UnsetIdle();
	char *fn=fl_file_chooser("Pick a Wav file to load", "*.wav", NULL);
		
	if (fn && fn!="")
	{
		m_loop->LoadWav(fn);
	}

  GUILoopGroup->SetData(m_loop->GetLoopPtr());
  GUILoopGroup->SetLength(m_loop->GetLoopLength());

  //LengthOutput->value(m_loop->GetLoopLength()/(float)SpiralLoopsInfo::SAMPLERATE);
  //GUILoopGroup->SetIdle();
  GUILoopGroup->redraw();
}
void LoopGUI::cb_Load(Fl_Button* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Load_i(o,v); }

inline void LoopGUI::cb_Play_i(Fl_LED_Button* o, void* v)
{ 
	m_loop->SetPlaying(o->value()); 
}
void LoopGUI::cb_Play(Fl_LED_Button* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Play_i(o,v); }

inline void LoopGUI::cb_Master_i(Fl_LED_Button* o, void* v)
{ 
}
void LoopGUI::cb_Master(Fl_LED_Button* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Master_i(o,v); }

inline void LoopGUI::cb_Effects_i(Fl_Button* o, void* v)
{
	Fl_Widget *p=GUILoopGroup->parent();

	if (o->value())
	{
		p->size(p->w(),p->h()+EFFECT_ADDITION);
	}
	else
	{
		p->size(p->w(),p->h()-EFFECT_ADDITION);
	}
	
	m_loop->SetEffects(o->value());
}
void LoopGUI::cb_Effects(Fl_Button* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Effects_i(o,v); }

inline void LoopGUI::cb_Update_i(Fl_LED_Button* o, void* v)
{
	GUILoopGroup->SetUpdate(o->value());
}
void LoopGUI::cb_Update(Fl_LED_Button* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Update_i(o,v); }

inline void LoopGUI::cb_Double_i(Fl_Button* o, void* v)
{
	m_loop->Double();
	
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	GUILoopGroup->redraw();

}
void LoopGUI::cb_Double(Fl_Button* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Double_i(o,v); }

inline void LoopGUI::cb_Match_i(Fl_Button* o, void* v)
{
}
void LoopGUI::cb_Match(Fl_Button* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Match_i(o,v); }

inline void LoopGUI::cb_Save_i(Fl_Button* o, void* v)
{
	char *fn=fl_file_chooser("Save loop as Wav file", "*.wav", NULL);
		
	if (fn && fn!="")
	{
		m_loop->SaveWav(fn);
	}
}
void LoopGUI::cb_Save(Fl_Button* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_Save_i(o,v); }

inline void LoopGUI::cb_WaveSize_i(Fl_Knob* o, void* v)
{
	GUILoopGroup->SetWaveSize(1+o->value());
	GUILoopGroup->redraw();
}
void LoopGUI::cb_WaveSize(Fl_Knob* o, void* v)
{ ((LoopGUI*)(o->parent()->user_data()))->cb_WaveSize_i(o,v); }


////////////////////////////////////////////////////////////////


inline void LoopGUI::cb_Cut_i(Fl_Loop *o, int Start, int End)
{
	m_loop->Cut(Start,End);
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	GUILoopGroup->redraw();
}
void LoopGUI::cb_Cut(Fl_Loop *o, int Start, int End)
{ ((LoopGUI*)(o->user_data()))->cb_Cut_i(o,Start,End); }

void LoopGUI::cb_Copy_i(Fl_Loop *o, int Start, int End)
{
	m_loop->Copy(Start,End);
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	GUILoopGroup->redraw();
}
void LoopGUI::cb_Copy(Fl_Loop *o, int Start, int End)
{ ((LoopGUI*)(o->user_data()))->cb_Copy_i(o,Start,End); }

inline void LoopGUI::cb_Paste_i(Fl_Loop *o, int Start)
{
	m_loop->Paste(Start);
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	GUILoopGroup->redraw();
}
void LoopGUI::cb_Paste(Fl_Loop *o, int Start)
{ ((LoopGUI*)(o->user_data()))->cb_Paste_i(o,Start); }
 
inline void LoopGUI::cb_PasteMix_i(Fl_Loop *o, int Start)
{
	m_loop->PasteMix(Start);
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	GUILoopGroup->redraw();
}
void LoopGUI::cb_PasteMix(Fl_Loop *o, int Start)
{ ((LoopGUI*)(o->user_data()))->cb_PasteMix_i(o,Start); }
 
 
inline void LoopGUI::cb_ZeroRange_i(Fl_Loop *o, int Start, int End)
{
	m_loop->ZeroRange(Start,End);
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	GUILoopGroup->redraw();
}
void LoopGUI::cb_ZeroRange(Fl_Loop *o, int Start, int End)
{ ((LoopGUI*)(o->user_data()))->cb_ZeroRange_i(o,Start,End); }

inline void LoopGUI::cb_ReverseRange_i(Fl_Loop *o, int Start, int End)
{
	m_loop->ReverseRange(Start,End);
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	GUILoopGroup->redraw();
}
void LoopGUI::cb_ReverseRange(Fl_Loop *o, int Start, int End)
{ ((LoopGUI*)(o->user_data()))->cb_ReverseRange_i(o,Start,End); }

inline void LoopGUI::cb_Halve_i(Fl_Loop *o, int Start, int End)
{
	m_loop->Halve();
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	GUILoopGroup->redraw();
}
void LoopGUI::cb_Halve(Fl_Loop *o, int Start, int End)
{ ((LoopGUI*)(o->user_data()))->cb_Halve_i(o,Start,End); }

inline void LoopGUI::cb_Hold_i(Fl_Loop *o, int Start, int End)
{
	m_loop->Hold();
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	GUILoopGroup->redraw();
}
void LoopGUI::cb_Hold(Fl_Loop *o, int Start, int End)
{ ((LoopGUI*)(o->user_data()))->cb_Hold_i(o,Start,End); }

inline void LoopGUI::cb_SelectAll_i(Fl_Loop *o, int Start, int End)
{
	m_loop->SelectAll();
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	GUILoopGroup->redraw();
}
void LoopGUI::cb_SelectAll(Fl_Loop *o, int Start, int End)
{ ((LoopGUI*)(o->user_data()))->cb_SelectAll_i(o,Start,End); }

inline void LoopGUI::cb_NewTrigger_i(Fl_Loop *o, int Start, int End)
{
	if (m_TriggerCount>=8) return;

	Fl_Trigger *NewTrigger = new Fl_Trigger(0, 0, 20, 20);
	NewTrigger->SetCentreX(150);
	NewTrigger->SetCentreY(170);
	NewTrigger->SetCentreRadius(125);
	NewTrigger->SetID(m_TriggerCount);
	
	GUILoopGroup->add(NewTrigger);
	m_TriggerVec.push_back(NewTrigger);
	GUILoopGroup->redraw();
	GUILoopGroup->parent()->redraw();
	
	m_TriggerCount++;
}
void LoopGUI::cb_NewTrigger(Fl_Loop *o, int Start, int End)
{ ((LoopGUI*)(o->user_data()))->cb_NewTrigger_i(o,Start,End); }

inline void LoopGUI::cb_Move_i(Fl_Loop *o, int Start, int End)
{
	m_loop->Move(Start);
	GUILoopGroup->SetData(m_loop->GetLoopPtr());
	GUILoopGroup->SetLength(m_loop->GetLoopLength());
	GUILoopGroup->redraw();
}
void LoopGUI::cb_Move(Fl_Loop *o, int Start, int End)
{ ((LoopGUI*)(o->user_data()))->cb_Move_i(o,Start,End); }

/////////////////////////////////////////////////////

istream &operator>>(istream &s, LoopGUI &o)
{
	int x,y,w,h;
	s>>x>>y>>w>>h;
	//o.GUILoopGroup->resize(x,y,w,h);

	int Num;
	s>>Num;
	
	for (int n=0; n<Num; n++)
	{
		Fl_Trigger *NewTrigger = new Fl_Trigger(0, 0, 20, 20);
		s>>*NewTrigger;
		o.GUILoopGroup->add(NewTrigger);
		o.m_TriggerVec.push_back(NewTrigger);
	}
	
	return s;	
}

ostream &operator<<(ostream &s, LoopGUI &o)
{
	s<<o.GUILoopGroup->GetX()<<" "<<o.GUILoopGroup->GetY()<<" "<<
	   o.GUILoopGroup->GetW()<<" "<<o.GUILoopGroup->GetH()<<" ";
	
	s<<o.m_TriggerVec.size()<<" ";
	
	for (vector<Fl_Trigger*>::iterator i=o.m_TriggerVec.begin();
		 i!=o.m_TriggerVec.end(); i++)
	{
		s<<**i;
	}
	 
	return s;
}
