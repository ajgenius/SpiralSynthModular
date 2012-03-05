/*  EventMap Widget
 *  Copyleft (C) 2001 David Griffiths <dave@pawfal.org>
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

#include "Fl_EventMap.h"
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>
#include <stdio.h>

static const int AREA_WIDTH = 1000;
static const int AREA_HEIGHT = 1000;
static const int BUTTON_COLOUR = 10;

static const int EVENT_COLOUR = 0;

int BG_COLOUR;// = 139;
int BLACK_KEY_COL;// = 141;
int LINE_COLOUR;// = 140;

////////////////////////////////////////////////////////////////////

Fl_EventMap::Fl_EventMap(int x, int y, int ww, int h, const char* label) :
Fl_Group(x,y,ww,h,label),
m_Type(ARRANGE_MAP),
m_Update(true),
m_Zoom(1.0f),
m_GridSizeX(0),
m_GridSizeY(10),
m_PixelsPerSec(100),
m_NextID(0),
m_StartLoop(1.0f),
m_EndLoop(2.0f),
m_Pos(0),
m_LastPos(0),
m_DrawBeats(true),
m_BarLength(1),
m_BeatsBar(4),
m_FirstUpdate(true)
{
	box(FL_FLAT_BOX);

	fl_color(0,100,161);
	BG_COLOUR=fl_color();
	fl_color(0,90,150);
	BLACK_KEY_COL=fl_color();
	fl_color(0,80,150);
	LINE_COLOUR=fl_color();
	
	fl_color(200,200,200);
// SpiralSound/Plugins/Widgets/Fl_EventMap.C:65: declaration of `w' shadows a parameter
	int w=fl_color();
	fl_color(50,50,50);
	int b=fl_color();
	
	// white/black keys for the melody
	m_KeyColMap[0] = w;
	m_KeyColMap[1] = b;
	m_KeyColMap[2] = w;
	m_KeyColMap[3] = b;
	m_KeyColMap[4] = w;
	m_KeyColMap[5] = w;
	m_KeyColMap[6] = b;
	m_KeyColMap[7] = w;
	m_KeyColMap[8] = b;
	m_KeyColMap[9] = w;
	m_KeyColMap[10] = b;
	m_KeyColMap[11] = w;
}

void Fl_EventMap::SetCallbacks(const EventCallbacks &s)
{ 
	m_Callbacks.cb_NewEvent=s.cb_NewEvent;
	m_Callbacks.cb_EventDoubleClicked=s.cb_EventDoubleClicked;
	m_Callbacks.cb_CopyEvent=s.cb_CopyEvent;
	m_Callbacks.cb_CloneEvent=s.cb_CloneEvent;
	m_Callbacks.cb_InstanceEvent=s.cb_InstanceEvent;
	m_Callbacks.cb_MoveEvent=s.cb_MoveEvent;
	m_Callbacks.cb_EditEvent=s.cb_EditEvent;
	m_Callbacks.cb_DelEvent=s.cb_DelEvent;
	m_Callbacks.cb_RenameEvent=s.cb_RenameEvent;
	m_Callbacks.cb_Recolour=s.cb_Recolour;
}

void Fl_EventMap::CreateWindow()
{
	show();
}

void Fl_EventMap::draw()
{
	Fl_Widget*const* a = array();
	if (!(damage() & ~FL_DAMAGE_CHILD)) 
	{
		for (int i=children(); i--;) update_child(**a++);
		return;
	}
	
	draw_box();
	m_FirstUpdate=true;
		
	fl_color(BG_COLOUR);
	fl_rectf(x(),y(),w(),h());
	// draw the section markers
	
	if (m_GridSizeY)
	{
		int c=0;
		// draw the snap points
		for (int n=y(); n<y()+h(); n+=m_GridSizeY)
		{
			fl_color(LINE_COLOUR);
			fl_line(x(),n,x()+w(),n);
			
			// do black key bg
			if (m_Type==MELODY_MAP)
			{
				int cm = c%12;
				if (cm==1 || cm==3 || cm==6 || cm==8 || cm==10)
				{
					fl_color(BLACK_KEY_COL);
					fl_rectf(x(),n,w(),m_GridSizeY);
				}			
			}
			c++;	
		}
	}

	// draw the bar markers	
	if (m_DrawBeats)
	{
		float BarLength=m_BarLength*(float)m_PixelsPerSec;
		float BeatLength=BarLength/(float)m_BeatsBar;
		
		float n=x();
		while (n<x()+w())
		{
			fl_color(LINE_COLOUR);
			fl_line((int)n,y(),(int)n,y()+h());
			if (m_Type==MELODY_MAP)
			{
				fl_line((int)n-1,y(),(int)n-1,y()+h());
			}
			
			if (m_Type!=ARRANGE_MAP)
			{
				// draw the beat markers
				float m=n;
				while(m<n+BarLength)
				{
					fl_color(LINE_COLOUR);
					fl_line((int)m,y(),(int)m,y()+h());
					m+=BeatLength;
				}
			}
			n+=BarLength;
		}
	}
	
	if (m_Type==ARRANGE_MAP)
	{
		fl_color(FL_BLACK);
			
		int StartPixels=(int)(m_StartLoop*(float)m_PixelsPerSec)+x();
		int EndPixels=(int)(m_EndLoop*(float)m_PixelsPerSec)+x();
		
		//fl_line(StartPixels,y()+15,StartPixels,y()+h());
		fl_line(EndPixels,y()+15,EndPixels,y()+h());
	}
	
	for (int i=children(); i--;) {
      Fl_Widget& o = **a++;
      draw_child(o);
      draw_outside_label(o);
    }
	
	SetFirstUpdate();
}

int Fl_EventMap::handle(int  event)
{	
	if (!Fl_Group::handle(event) && event==FL_PUSH)
	{	
		if (m_Type!=ARRANGE_MAP)
		{
			AddEvent(Fl::event_x(),Fl::event_y(),20, Fl_SEvent::NO_TYPE);
		}
	}

	for (map<int,Fl_SEvent*>::iterator i=m_EventMap.begin();
		 i!=m_EventMap.end(); i++)
	{
		Fl_SEvent *pEvent = i->second;

		if (pEvent->Killed())
		{
			RemoveEvent(i->first);
		}
	}
	
	return event;
}

void Fl_EventMap::SetZoomLevel(float s) 
{ 
	m_Zoom=s; 
	m_PixelsPerSec=int(100/m_Zoom);
	
	vector<EventInfo> EventVec;
	for (map<int,Fl_SEvent*>::iterator i=m_EventMap.begin();
		 i!=m_EventMap.end(); i++)
	{
		i->second->SetPixelsPerSec(m_PixelsPerSec);
	}
		
	redraw();
}
	
void Fl_EventMap::SetSnapGap(float s)
{
	m_GridSizeX=(int)(s*(float)m_PixelsPerSec);

	for (map<int,Fl_SEvent*>::iterator i=m_EventMap.begin();
		 i!=m_EventMap.end(); i++)
	{
		i->second->SetSnapGap(s);
	}
		
	redraw();
}

void Fl_EventMap::SetTime(float Time)
{	
	assert(m_PixelsPerSec!=0);
	
	Time -= m_TimeOffset;
	
	m_Pos = (int)(Time*m_PixelsPerSec);
	
	// line marker update 
	// all in all, some orrible code...
	if (m_Update && window()->visible() && visible_r() && m_Pos!=m_LastPos)
	{
		window()->make_current();
		int DrawPos=m_Pos+x();
		
		int Left=parent()->x();
		int Top=parent()->y();
		int Width=parent()->w();
		int Depth=parent()->h();
				
		if (DrawPos>Left && DrawPos<Left+Width)
		{
#if !__APPLE__
  			XSetFunction(fl_display,fl_gc,GXxor);
  			XSetForeground(fl_display, fl_gc, 0xff00ffff);
			if (!m_FirstUpdate) fl_line(m_LastPos,Top,m_LastPos,Depth);
			fl_line(DrawPos,Top,DrawPos,Depth);
			XSetFunction(fl_display,fl_gc,GXcopy);
#endif
			m_LastPos=DrawPos;
		}
	}
	m_FirstUpdate=false;
}

vector<EventInfo> Fl_EventMap::GetEvents(float Time)
{
	SetTime(Time);
	
	vector<EventInfo> EventVec;
	for (map<int,Fl_SEvent*>::iterator i=m_EventMap.begin();
		 i!=m_EventMap.end(); i++)
	{
		Fl_SEvent *pEvent = i->second;
		
		bool State=pEvent->UpdateState(Time);
	
		EventInfo::Type type=EventInfo::ON;
			
		if (pEvent->AtStart()) type=EventInfo::START;
		if (pEvent->AtEnd())   type=EventInfo::END;
	
		// Collect the events happening now, or just finished
		if(State || pEvent->AtEnd())
		{
			EventVec.push_back(EventInfo(pEvent->GetID(),pEvent->GetGroup(),
						type,pEvent->GetStartTime()));
		}
	}
	return EventVec;
}

int Fl_EventMap::GetGroupFromY(int y)
{
	return y/m_GridSizeY;
}

int Fl_EventMap::AddEventTime(float st, int g, float lt, Fl_SEvent::Type EType, bool CallBack)
{
	int ex,ey,ew;
	ex=(int)(st*(float)m_PixelsPerSec)+x();
	ew=(int)(lt*(float)m_PixelsPerSec);
	ey=g*m_GridSizeY+y();

	return AddEvent(ex, ey, ew, EType, CallBack);
}

int Fl_EventMap::AddEvent(int x, int y, int w, Fl_SEvent::Type EType, bool CallBack)
{	
	Fl_SEvent *NewEvent;
	if (m_Type==PERCUSSION_MAP)
	{
		NewEvent = new Fl_CircEvent(x,y,w,m_GridSizeY);
	}
	else
	{
		NewEvent = new Fl_SEvent(x,y,w,m_GridSizeY);
	}
	
	if (m_Type==MELODY_MAP) NewEvent->LockResize(false);
	
	NewEvent->SetPixelsPerSec(m_PixelsPerSec, true);
	NewEvent->color(EVENT_COLOUR);
	NewEvent->labelsize(10);
	NewEvent->SetID(m_NextID++);
	
	NewEvent->SetType(EType);
	char Name[256];
	if (m_Type==ARRANGE_MAP) 
	{
		if(NewEvent->GetType()==Fl_SEvent::MELODY)
			sprintf(Name,"Melody %d",NewEvent->GetID());	
		
		if(NewEvent->GetType()==Fl_SEvent::PERCUSSION)
			sprintf(Name,"Percussion %d",NewEvent->GetID());	
		
	}
	else sprintf(Name,"This shouldnt be read!");	
	
	NewEvent->SetName(Name);
	NewEvent->SetGridX(m_GridSizeX);
	NewEvent->SetGridY(m_GridSizeY);
	
	map<int,Fl_SEvent*>::iterator i=m_EventMap.find(NewEvent->GetID());
	if (i!=m_EventMap.end())
	{
		cerr<<"Event ID not unique, not added!"<<endl;
		delete NewEvent;
		return -1;
	}
	
	add(NewEvent);

	// can only build the menus after adding to the parent, as the features in the
	// menu depend on which callbacks have been filled out by the parent.
	NewEvent->BuildMenu();

	NewEvent->SnapX();
	NewEvent->SnapY();
	
	NewEvent->SetPixelsPerSec(m_PixelsPerSec,true);
	
	m_EventMap[NewEvent->GetID()]=NewEvent;
	
	int pID = NewEvent->GetID();
	if (m_Callbacks.cb_NewEvent && CallBack) m_Callbacks.cb_NewEvent(NewEvent,&pID);
	
	redraw();
	return NewEvent->GetID();
}

int Fl_EventMap::CopyEvent(int x, int y, int w, int ID, float LengthTime)
{
	// make the new widget
	int NewID = AddEvent(x,y,w,m_EventMap[ID]->GetType(),false);
	
	// set the length of time exactly, as rounding 
	// errors cause a reinit of the sample memory
	GetEvent(NewID)->SetLengthTime(LengthTime);
	
	//string Name="Copy of "+m_EventMap[ID]->GetName();
	//GetEvent(NewID)->SetName(Name);
	
	GetEvent(NewID)->SetName(m_EventMap[ID]->GetName());
	GetEvent(NewID)->SetColour(m_EventMap[ID]->GetColour());
	GetEvent(NewID)->SetChannel(m_EventMap[ID]->GetChannel());
	
	return NewID;
}

void Fl_EventMap::RemoveAllEvents()
{
	for (map<int,Fl_SEvent*>::iterator i=m_EventMap.begin();
		 i!=m_EventMap.end(); i++)
	{
		Fl_SEvent *e=i->second;
		remove(e);
		m_EventMap.erase(i);
		delete(e);
	}	
	m_EventMap.clear();
	m_NextID=0;
	
	redraw();
}	

void Fl_EventMap::RemoveEvent(int ID)
{
	map<int,Fl_SEvent*>::iterator i=m_EventMap.find(ID);
	if (i==m_EventMap.end())
	{
		cerr<<"Event ID not found to delete"<<endl;
		return;
	}	
	
	Fl_SEvent *e=i->second;
	remove(e);
	m_EventMap.erase(i);
	delete(e);
	
	cerr<<"removed "<<ID<<endl;
}	

Fl_SEvent *Fl_EventMap::GetEvent(int ID)
{
	map<int,Fl_SEvent*>::iterator i=m_EventMap.find(ID);
	if (i==m_EventMap.end())
	{
		cerr<<"Event ID not found"<<endl;
		return NULL;
	}	
	
	return m_EventMap[ID];
}	

void Fl_EventMap::SetEventLength(int ID, float l) 
{ 
	Fl_SEvent *e=GetEvent(ID);
	if(e) e->SetLengthTime(l); 
}

void Fl_EventMap::CopyFrom(Fl_EventMap *Other)
{
	m_Type         = Other->m_Type;
	m_Zoom         = Other->m_Zoom;
	m_GridSizeX    = Other->m_GridSizeX;
	m_GridSizeY    = Other->m_GridSizeY;
	m_PixelsPerSec = Other->m_PixelsPerSec;
	m_StartLoop    = Other->m_StartLoop;
	m_EndLoop      = Other->m_EndLoop;
	m_Pos          = Other->m_Pos;
	m_LastPos      = Other->m_LastPos;
	m_BarLength    = Other->m_BarLength;
	m_BeatsBar     = Other->m_BeatsBar;
	m_NextID	   = Other->m_NextID;
	
	for (map<int,Fl_SEvent*>::iterator i=Other->m_EventMap.begin();
		 i!=Other->m_EventMap.end(); i++)
	{
		Fl_SEvent *NewEvent;

		if (m_Type==PERCUSSION_MAP)
		{
			NewEvent = new Fl_CircEvent(*(Fl_CircEvent*)i->second);
		}
		else
		{
			NewEvent = new Fl_SEvent(*i->second);
		}

		add(NewEvent);	
		m_EventMap[NewEvent->GetID()]=NewEvent;
	}
}

/////////////////////////////////////////////////////

istream &operator>>(istream &s, Fl_EventMap &o)
{
	string dummy;
	s>>dummy;
	if (dummy!="EventMap") cerr<<"Error in stream before Fl_EventMap"<<endl;
	s>>(int&)o.m_Type;
	s>>o.m_Zoom;
	s>>o.m_GridSizeX;
	s>>o.m_GridSizeY;
	s>>o.m_PixelsPerSec;
	s>>o.m_StartLoop;
	s>>o.m_EndLoop;
	s>>o.m_Pos;
	s>>o.m_LastPos;
	s>>o.m_BarLength;
	s>>o.m_BeatsBar;
		
	int Num;
	
	s>>Num;
	float st,lt;
	int g,t;
	
	for (int n=0; n<Num; n++)
	{	
		s>>st>>g>>lt>>t;	
		o.AddEventTime(st,g,lt,(Fl_SEvent::Type)(t), false);
	}
		
	return s;	
}

ostream &operator<<(ostream &s, Fl_EventMap &o)
{ 
	s<<"EventMap ";
	s<<(int)o.m_Type<<" ";
	s<<o.m_Zoom<<" ";
	s<<o.m_GridSizeX<<" ";
	s<<o.m_GridSizeY<<" ";
	s<<o.m_PixelsPerSec<<" ";
	s<<o.m_StartLoop<<" ";
	s<<o.m_EndLoop<<" ";
	s<<o.m_Pos<<" ";
	s<<o.m_LastPos<<" ";
	s<<o.m_BarLength<<" ";
	s<<o.m_BeatsBar<<" ";
	
	s<<o.m_EventMap.size()<<" ";
	
	for (map<int,Fl_SEvent*>::iterator i=o.m_EventMap.begin();
		 i!=o.m_EventMap.end(); i++)
	{
		s<<i->second->GetStartTime()<<" "<<i->second->GetGroup()<<" "<<
		i->second->GetLengthTime()<<" "<<(int)(i->second->GetType())<<" ";
	}
	s<<endl;
	
	return s;
}

