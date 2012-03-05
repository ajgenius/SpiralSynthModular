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
#include "SequencerPluginGUI.h"
#include <FL/fl_draw.H>
#include <FL/fl_draw.H>
#include <FL/fl_file_chooser.H>
#include <FL/Fl_Color_Chooser.H>

using namespace std;

////////////////////////////////////////////

SequencerPluginGUI::PatternWin::PatternWin(int w,int h,const char* n) :
Fl_Double_Window(w,h,n)
{
	m_Scroll = new Fl_Scroll(0, 0, w, h, "");
	resizable(m_Scroll);
	add(m_Scroll);
	m_NextNoteID=0;
	
	m_Melody = new Fl_EventMap(0, 0, 1000, 1000, "");
	m_Melody->SetType(Fl_EventMap::MELODY_MAP);
	m_Scroll->add(m_Melody);
	m_Scroll->position(0,350);
	m_Melody->CreateWindow();
	m_Melody->show();
	
	// callbacks for note events
	Fl_EventMap::EventCallbacks cb;
	cb.cb_NewEvent=(Fl_Callback*)cb_NewNote;
	cb.cb_MoveEvent=(Fl_Callback*)cb_MoveNote;
	cb.cb_DelEvent=(Fl_Callback*)cb_RemoveNote;
		
	m_Melody->SetCallbacks(cb);
	
	end();
}

////////////////////////////////////////////

SequencerPluginGUI::SequencerPluginGUI(int w, int h,SequencerPlugin *o,ChannelHandler *ch,const HostInfo *Info) :
SpiralPluginGUI(w,h,o,ch)
{		
	m_Scroll = new Fl_Scroll(50, 20, w-57, h-26, "");
	add(m_Scroll);
	m_ArrangementMap = new Fl_EventMap(0, 0, 1000, 132*10, "");
	//                    num midi notes * gridsize y ^	
	m_ArrangementMap->user_data((void*)this);
	m_ArrangementMap->SetType(Fl_EventMap::ARRANGE_MAP);
	m_Scroll->add(m_ArrangementMap);
	m_Scroll->position(-50,350);
	m_ArrangementMap->CreateWindow();
	m_ArrangementMap->SetZoomLevel(3.0f);
	m_ArrangementMap->show();
	
	// callbacks for sequence events
	Fl_EventMap::EventCallbacks cb;
	cb.cb_EventDoubleClicked=(Fl_Callback*)cb_ArrangeRM;
	cb.cb_MoveEvent=(Fl_Callback*)cb_MoveSequence;
	cb.cb_RenameEvent=(Fl_Callback*)cb_Rename;
	cb.cb_Recolour=(Fl_Callback*)cb_Recolour;
	cb.cb_CopyEvent=(Fl_Callback*)cb_Copy;
	cb.cb_DelEvent=(Fl_Callback*)cb_RemoveSequence;
	cb.cb_EditEvent=(Fl_Callback*)cb_Edit;
	
	m_ArrangementMap->SetCallbacks(cb);
	
	m_Length = new Fl_Input(12, 15, 24, 12, "LengthBars");
    m_Length->color(Info->GUI_COLOUR);
    m_Length->labelsize(8);
    m_Length->align(FL_ALIGN_BOTTOM|FL_ALIGN_CENTER);
    m_Length->textsize(8);
    m_Length->value("4");
	m_Length->callback((Fl_Callback*)cb_Length);
	add(m_Length);
	
	m_BeatsPerBar = new Fl_Input(12, 38, 24, 12, "BeatsPerBar");
    m_BeatsPerBar->color(Info->GUI_COLOUR);
    m_BeatsPerBar->labelsize(8);
	m_BeatsPerBar->align(FL_ALIGN_BOTTOM|FL_ALIGN_CENTER);
    m_BeatsPerBar->textsize(8);
    m_BeatsPerBar->value("4");
	m_BeatsPerBar->callback((Fl_Callback*)cb_BeatsPerBar);
	add(m_BeatsPerBar);
	
	m_BarLength = new Fl_Knob(5, 60, 40, 40, "BarLength");
    m_BarLength->color(Info->GUI_COLOUR);
	m_BarLength->type(Fl_Knob::DOTLIN);
    m_BarLength->labelsize(10);
    m_BarLength->maximum(10);
    m_BarLength->step(0.01);
    m_BarLength->value(1.0);
	m_BarLength->callback((Fl_Callback*)cb_BarLength);
	add(m_BarLength);
	
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
    m_Zoom->maximum(6);
    m_Zoom->step(0.01);
    m_Zoom->value(3.0);   
	m_Zoom->callback((Fl_Callback*)cb_Zoom);
	add(m_Zoom);
	
	m_NewPattern = new Fl_Button(2,225,45,20,"New Pat");
	m_NewPattern->type(0);
	m_NewPattern->labelsize(10);
	m_NewPattern->callback((Fl_Callback*)cb_NewPattern,NULL);
	add(m_NewPattern);
	
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

void SequencerPluginGUI::LoadPatternData(int ID)
{	
	Note note(0,1.0f,0,0);

	m_PatternWinMap[ID]->GetEventMap()->RemoveAllEvents();
	
	m_GUICH->Wait();
	m_GUICH->Set("ID",ID);
	m_GUICH->SetCommand(SequencerPlugin::GET_PATTERN);
	m_GUICH->Wait();
	
	m_GUICH->RequestChannelAndWait("TransCount");
	int c = m_GUICH->GetInt("TransCount");
	cerr<<"TransCount="<<c<<endl;
	
	for (int n=0; n<c; n++)
	{
		m_GUICH->SetCommand(SequencerPlugin::GET_PATTERN);
		m_GUICH->RequestChannelAndWait("TransNote");
		m_GUICH->GetData("TransNote",(void*)&note);
	
	cerr<<"Adding note "<<n<<" "<<note.Time<<" "<<note.MidiNote<<" "<<note.Length<<endl;
	
		m_PatternWinMap[ID]->GetEventMap()->AddEventTime(note.Time,note.MidiNote,note.Length,Fl_SEvent::MELODY,false);
	}
}

void SequencerPluginGUI::UpdateValues(SpiralPlugin *o)
{
	SequencerPlugin *Plugin = (SequencerPlugin *)o;
	Track *t = Plugin->GetTrack();
	map<int,Sequence> *seqmap=t->GetSequenceMap();
	map<int,Note>::iterator pi;
		
	// for each sequence
	for (map<int,Sequence>::iterator i=seqmap->begin();
	     i!=seqmap->end(); i++)
	{
		int eid = m_ArrangementMap->AddEventTime(i->second.GetStartTime(),i->second.GetYPos(),i->second.GetLength(),Fl_SEvent::NO_TYPE,false);
		m_ArrangementMap->GetEvent(eid)->SetName(i->second.GetName());
		m_ArrangementMap->GetEvent(eid)->SetColour(i->second.GetColour());
		m_ArrangementMap->GetEvent(eid)->SetChannel(i->second.GetChannel());
		m_ArrangementMap->GetEvent(eid)->SetLengthTime(i->second.GetLength());
		m_PatternWinMap[eid] = new PatternWin(400,200,m_ArrangementMap->GetEvent(eid)->GetName().c_str());
	
		Fl_EventMap *map = m_PatternWinMap[eid]->GetEventMap();	
		map->user_data((void*)this);
		map->SetID(eid);
		
		// load the pattern data
		Pattern *p = t->GetPattern(i->second.GetPatternID());
				
		for (pi=p->m_NoteMap.begin();
	     	 pi!=p->m_NoteMap.end(); pi++)
		{
			map->AddEventTime(pi->second.Time,pi->second.MidiNote,pi->second.Length,Fl_SEvent::MELODY,false);
		}
	} 
	
	redraw();
}
	
void SequencerPluginGUI::Update()
{
	float Time=m_GUICH->GetFloat("CurrentTime");
	m_ArrangementMap->SetTime(Time);
	
	for (map<int,PatternWin*>::iterator i=m_PatternWinMap.begin();
		 i!=m_PatternWinMap.end(); i++)
	{
		i->second->GetEventMap()->SetTime(Time);
	}	
}

void SequencerPluginGUI::ChangeSequenceHelper(Fl_SEvent *event)
{
	m_GUICH->Set("ID",event->GetID());
	m_GUICH->Set("ID2",event->GetColour());
	char t[256];
	sprintf(t,"%s",event->GetName().c_str());
	m_GUICH->SetData("Name",(void*)t);
	m_GUICH->Set("Length",event->GetLengthTime());
	m_GUICH->Set("Note",event->GetGroup());
	m_GUICH->Set("Channel",event->GetChannel());
	m_GUICH->Set("Time",event->GetStartTime());	
	m_GUICH->SetCommand(SequencerPlugin::CHG_SEQ);
}

inline void SequencerPluginGUI::cb_NoteCut_i(Fl_Button* o, void* v)
{ 
	//m_Plugin->SetNoteCut(o->value());
}
void SequencerPluginGUI::cb_NoteCut(Fl_Button* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()))->cb_NoteCut_i(o,v);}

inline void SequencerPluginGUI::cb_Zoom_i(Fl_Knob* o, void* v)
{
	if (o->value()!=0) m_ArrangementMap->SetZoomLevel(o->value()); 
}
void SequencerPluginGUI::cb_Zoom(Fl_Knob* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()))->cb_Zoom_i(o,v);}



inline void SequencerPluginGUI::cb_Pattern_i(Fl_Counter* o, void* v)
{ 	
	if (o->value()<0) o->value(0);
	if (o->value()>NUM_PATTERNS-1) o->value(NUM_PATTERNS-1);
	//m_Plugin->SetPattern((int)o->value());
	//UpdateValues();
}
void SequencerPluginGUI::cb_Pattern(Fl_Counter* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()))->cb_Pattern_i(o,v);}

inline void SequencerPluginGUI::cb_Length_i(Fl_Input* o, void* v)
{
	int val=(int)strtod(o->value(),NULL);
	if (val<1) o->value("error!");
	else
	{
		m_GUICH->Set("TotalLength",val);
	}
}
void SequencerPluginGUI::cb_Length(Fl_Input* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()))->cb_Length_i(o,v);}

inline void SequencerPluginGUI::cb_BeatsPerBar_i(Fl_Input* o, void* v)
{
	int val=(int)strtod(o->value(),NULL);
	if (val<1) o->value("error!");
	else
	{
		m_GUICH->Set("BeatsPerBar",val);
		m_ArrangementMap->SetBeatsBar(val);
		m_ArrangementMap->redraw();
		
		for (map<int,PatternWin*>::iterator i=m_PatternWinMap.begin();
		 	 i!=m_PatternWinMap.end(); i++)
		{
			i->second->GetEventMap()->SetBeatsBar(val);
			i->second->GetEventMap()->redraw();
		}
	}
}
void SequencerPluginGUI::cb_BeatsPerBar(Fl_Input* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()))->cb_BeatsPerBar_i(o,v);}

inline void SequencerPluginGUI::cb_BarLength_i(Fl_Knob* o, void* v)
{
	if (o->value()!=0) 
	{
		m_GUICH->Set("BarLength",(float)o->value());
		m_ArrangementMap->SetBarLength(o->value());
		m_ArrangementMap->redraw();
	
		for (map<int,PatternWin*>::iterator i=m_PatternWinMap.begin();
			 i!=m_PatternWinMap.end(); i++)
		{
			i->second->GetEventMap()->SetBarLength(o->value());
			i->second->GetEventMap()->redraw();
		}
	
		redraw();
	}
}
void SequencerPluginGUI::cb_BarLength(Fl_Knob* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()))->cb_BarLength_i(o,v);}

inline void SequencerPluginGUI::cb_Speed_i(Fl_Knob* o, void* v)
{ 	
	//m_Plugin->SetSpeed(o->value()-2.0f);
}
void SequencerPluginGUI::cb_Speed(Fl_Knob* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()))->cb_Speed_i(o,v);}

inline void SequencerPluginGUI::cb_Clear_i(Fl_Button* o, void* v)
{ 	
	//m_Plugin->ClearAll();
}
void SequencerPluginGUI::cb_Clear(Fl_Button* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()))->cb_Clear_i(o,v);}

/// sequence callbacks /////////////////////////////////////////////////////////

inline void SequencerPluginGUI::cb_NewPattern_i(Fl_Button* o, void* v)
{
	int eid = m_ArrangementMap->AddEventTime(1,50,2,Fl_SEvent::NO_TYPE);
	m_ArrangementMap->GetEvent(eid)->SetName("My Pattern");
	m_PatternWinMap[eid] = new PatternWin(400,200,m_ArrangementMap->GetEvent(eid)->GetName().c_str());
	
	Fl_EventMap *map = m_PatternWinMap[eid]->GetEventMap();
	Fl_SEvent *event = m_ArrangementMap->GetEvent(eid);
	
	// setup the stuff needed by the callbacks
	map->user_data((void*)this);
	map->SetID(eid);
	
	m_GUICH->Set("ID",eid);
	m_GUICH->Set("ID2",event->GetColour());
	char t[256];
	sprintf(t,"%s",event->GetName().c_str());
	m_GUICH->SetData("Name",(void*)t);	
	m_GUICH->Set("Length",event->GetLengthTime());
	m_GUICH->Set("Channel",event->GetChannel());
	m_GUICH->Set("Note",event->GetGroup());
	m_GUICH->SetCommand(SequencerPlugin::NEW_SEQ);
	
	redraw();
}
void SequencerPluginGUI::cb_NewPattern(Fl_Button* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()))->cb_NewPattern_i(o,v);}

inline void SequencerPluginGUI::cb_Rename_i(Fl_Widget* o, void* v)
{ 	
	Fl_SEvent *event = ((Fl_SEvent*)v);	

	const char *name = fl_input("Rename the sequence:", 
		m_ArrangementMap->GetEvent(event->GetID())->GetName().c_str());
	if (name) m_ArrangementMap->GetEvent(event->GetID())->SetName(name);
	
	ChangeSequenceHelper(event);	
	redraw();
}

void SequencerPluginGUI::cb_Rename(Fl_Widget* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()->parent()->user_data()))->cb_Rename_i(o,v);}

inline void SequencerPluginGUI::cb_Recolour_i(Fl_Widget* o, void* v)
{ 	
	Fl_SEvent *event = ((Fl_SEvent*)v);	

	unsigned char r=255,g=255,b=255;
	if (fl_color_chooser("colour",r,g,b))
	{
		fl_color(r,g,b);
		Fl_Color col=fl_color();
		m_ArrangementMap->GetEvent(event->GetID())->SetColour(col);
		ChangeSequenceHelper(event);	
		redraw();	
	}	
}
void SequencerPluginGUI::cb_Recolour(Fl_Widget* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()->parent()->user_data()))->cb_Recolour_i(o,v);}

inline void SequencerPluginGUI::cb_RemoveSequence_i(Fl_Widget* o, void* v)
{ 	
	Fl_SEvent *event = ((Fl_SEvent*)v);	
	Fl_EventMap *map = ((Fl_EventMap*)event->parent());
	
	event->KillMe();
	map->redraw();
	
	m_GUICH->Set("ID",event->GetID());
	m_GUICH->SetCommand(SequencerPlugin::REM_SEQ);
}
void SequencerPluginGUI::cb_RemoveSequence(Fl_Widget* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()->parent()->user_data()))->cb_RemoveSequence_i(o,v);}

inline void SequencerPluginGUI::cb_Copy_i(Fl_Widget* o, void* v)
{ 	
	Fl_SEvent *event = ((Fl_SEvent*)v);	
	int NewID = m_ArrangementMap->CopyEvent(event->x()+event->w(),event->y(),event->w(),event->GetID(), event->GetLengthTime());
	
	m_PatternWinMap[NewID] = new PatternWin(400,200,m_ArrangementMap->GetEvent(NewID)->GetName().c_str());
	m_PatternWinMap[NewID]->GetEventMap()->user_data((void*)this);
	m_PatternWinMap[NewID]->GetEventMap()->SetID(NewID);
	
	m_GUICH->Set("ID",event->GetID());
	m_GUICH->Set("ID2",NewID);
		
	m_GUICH->SetCommand(SequencerPlugin::COPY_SEQ);
	
	LoadPatternData(NewID);
}
void SequencerPluginGUI::cb_Copy(Fl_Widget* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()->parent()->user_data()))->cb_Copy_i(o,v);}

inline void SequencerPluginGUI::cb_MoveSequence_i(Fl_Widget* o, void* v)
{ 	
	Fl_SEvent *event = ((Fl_SEvent*)o);	
	m_PatternWinMap[event->GetID()]->GetEventMap()->SetTimeOffset(event->GetStartTime());
	ChangeSequenceHelper(event);
}
void SequencerPluginGUI::cb_MoveSequence(Fl_Widget* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()->user_data()))->cb_MoveSequence_i(o,v);}

inline void SequencerPluginGUI::cb_ArrangeRM_i(Fl_Button* o, void* v)
{ 	
	int ID=((Fl_SEvent*)o)->GetID();
		
	if (m_PatternWinMap[ID]->shown()) m_PatternWinMap[ID]->hide();
	else m_PatternWinMap[ID]->show();
}
void SequencerPluginGUI::cb_ArrangeRM(Fl_Button* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()->user_data()))->cb_ArrangeRM_i(o,v);}
#include <unistd.h>
inline void SequencerPluginGUI::cb_Edit_i(Fl_Widget* o, void* v)
{ 	
	Fl_SEvent *event = ((Fl_SEvent*)v);
		
	Fl_Double_Window *EditWin = new Fl_Double_Window(100,100,"Properties");
	Fl_Button *exit = new Fl_Button(10,80,80,10,"Exit");
	exit->labelsize(8);
	Fl_Counter *channel = new Fl_Counter(5,10,90,20,"Channel");
	channel->labelsize(8);
	channel->textsize(8);		
	channel->type(FL_SIMPLE_COUNTER);
	channel->step(1);
	channel->value(event->GetChannel());
	EditWin->show();
	while (!exit->value() || !EditWin->shown())
	{
		Fl::check();
		usleep(10000);
	}
	event->SetChannel((int)channel->value());
	ChangeSequenceHelper(event);	
	redraw();	
	
	EditWin->hide();
}
void SequencerPluginGUI::cb_Edit(Fl_Widget* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()->parent()->user_data()))->cb_Edit_i(o,v);}

/// note callbacks ///////////////////////////////////////////////////////////////

inline void SequencerPluginGUI::cb_NewNote_i(Fl_Widget* o, void* v)
{ 	
	Fl_SEvent *event = ((Fl_SEvent*)o);
	Fl_EventMap *map = ((Fl_EventMap*)o->parent());
	
	m_GUICH->Set("ID",event->GetID());
	m_GUICH->Set("Sequence",map->GetID());
	m_GUICH->Set("Time",event->GetStartTime());
	m_GUICH->Set("Length",event->GetLengthTime());
	m_GUICH->Set("Note",event->GetGroup());
	m_GUICH->Set("Vol",1.0f);
	
	m_GUICH->SetCommand(SequencerPlugin::NEW_NOTE);
}
void SequencerPluginGUI::cb_NewNote(Fl_Widget* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()->user_data()))->cb_NewNote_i(o,v);}

inline void SequencerPluginGUI::cb_MoveNote_i(Fl_Widget* o, void* v)
{ 	
	Fl_SEvent *event = ((Fl_SEvent*)o);	
	Fl_EventMap *map = ((Fl_EventMap*)o->parent());
		
	m_GUICH->Set("ID",event->GetID());
	m_GUICH->Set("Sequence",map->GetID());
	m_GUICH->Set("Time",event->GetStartTime());
	m_GUICH->Set("Length",event->GetLengthTime());
	m_GUICH->Set("Note",event->GetGroup());
	m_GUICH->Set("Vol",1.0f);

	m_GUICH->SetCommand(SequencerPlugin::CHG_NOTE);
}
void SequencerPluginGUI::cb_MoveNote(Fl_Widget* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()->user_data()))->cb_MoveNote_i(o,v);}

inline void SequencerPluginGUI::cb_RemoveNote_i(Fl_Widget* o, void* v)
{ 	
	Fl_SEvent *event = ((Fl_SEvent*)v);	
	Fl_EventMap *map = ((Fl_EventMap*)event->parent());
	
	event->KillMe();
	map->redraw();
	
	m_GUICH->Set("ID",event->GetID());
	m_GUICH->Set("Sequence",map->GetID());
	m_GUICH->SetCommand(SequencerPlugin::REM_NOTE);
}
void SequencerPluginGUI::cb_RemoveNote(Fl_Widget* o, void* v)
{ ((SequencerPluginGUI*)(o->parent()->parent()->user_data()))->cb_RemoveNote_i(o,v);}

