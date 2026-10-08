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

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "Fl_SEvent.h"


#ifndef EVENTMAP
#define EVENTMAP

using namespace std;

/////////////////////////////////////////////////////////

class EventInfo
{
public:
	enum Type{START,ON,END};

	EventInfo(int ID, int Group, Type t, float s) 
		{m_ID=ID; m_Group=Group; m_Type=t; m_SectionStart=s;}
	int m_ID;
	int m_Group;
	Type m_Type;
	float m_SectionStart;
};

/////////////////////////////////////////////////////////

class Fl_EventMap : public Fl_Group
{
public:

	enum ModeType {ARRANGE_MAP,MELODY_MAP,PERCUSSION_MAP};

	Fl_EventMap(int x, int y, int w, int h, const char* label=0);
	
	virtual void draw();
	virtual int  handle(int  event);
	void CreateWindow();
	
	vector<EventInfo> GetEvents(float Time);
	
	void SetTime(float Time);
	void SetUpdate(bool s) {m_Update=s;}
	void SetZoomLevel(float s);
	void SetGridSizeX(int s)  { m_GridSizeX=s; }
	void SetGridSizeY(int s)  { m_GridSizeY=s; }

	void SetType(ModeType s)       { m_Type=s; }
	ModeType GetType()             { return m_Type; }

	void  SetID(int ID)            { m_ID=ID; }
	int   GetID()                  { return m_ID; }
	float GetStartTime()           { return m_StartLoop;}
	float GetEndTime()             { return m_EndLoop;}
	void  SetEndTime(float s)      { m_EndLoop=s;}
	float GetTimeOffset()          { return m_TimeOffset;}
	void  SetTimeOffset(float s)   { m_TimeOffset=s;}
	void  SetBeatsBar(int s)       { m_BeatsBar=s; redraw();}
	void  SetBarLength(float s)    { m_BarLength=s; redraw(); }
	void  SetSnapGap(float s);
	int  *GetKeyColMap()           { return m_KeyColMap; }
	int   GetX()                   { return x(); }
	int   GetY()                   { return y(); }
    void  DrawBeats(bool s)        { m_DrawBeats=s; redraw(); }   
	int   AddEventTime(float st, int g, float lt, Fl_SEvent::Type EType, bool CallBack=true);
	void  SetEventLength(int ID, float l);
	int   AddEvent(int x, int y, int w, Fl_SEvent::Type EType, bool CallBack=true);
	int   CopyEvent(int x, int y, int w, int ID, float LengthTime);
	void  RemoveAllEvents();
	void  RemoveEvent(int ID);
	Fl_SEvent *GetEvent(int ID);
	void  CopyFrom(Fl_EventMap *Other);
	
	// called by the event widgets on the owner of this object
	// if you're going to hack callbacks - do it in style ;)
	class EventCallbacks
	{
		public:
		EventCallbacks()
		{
			cb_NewEvent=cb_EventDoubleClicked=cb_CopyEvent=cb_CloneEvent=
			cb_InstanceEvent=cb_MoveEvent=cb_EditEvent=cb_DelEvent=
			cb_RenameEvent=cb_Recolour=NULL;
		}
		
		void (*cb_NewEvent)(Fl_Widget*, void*);
		void (*cb_EventDoubleClicked)(Fl_Widget*, void*);
		void (*cb_CopyEvent)(Fl_Widget*, void*);
		void (*cb_CloneEvent)(Fl_Widget*, void*);
		void (*cb_InstanceEvent)(Fl_Widget*, void*);
		void (*cb_MoveEvent)(Fl_Widget*, void*);
		void (*cb_EditEvent)(Fl_Widget*, void*);
		void (*cb_DelEvent)(Fl_Widget*, void*);
		void (*cb_RenameEvent)(Fl_Widget*, void*);
		void (*cb_Recolour)(Fl_Widget*, void*);
	};
	
	EventCallbacks m_Callbacks;
	void  SetCallbacks(const EventCallbacks &s);

	void  TriggerStart();
	void  TriggerEnd();

	void SetFirstUpdate() { m_FirstUpdate=true; }
		
protected:

	int  GetGroupFromY(int y);
	
private:
	ModeType m_Type;
	bool  m_Update;
	float m_Zoom;
	int   m_ID;
	int   m_GridSizeX;
	int   m_GridSizeY;
	int   m_PixelsPerSec;
	int   m_NextID;
	float m_StartLoop;
	float m_EndLoop;
	int   m_Pos;
	int   m_LastPos;	
	bool  m_DrawBeats;
	float m_BarLength;
	float m_TimeOffset;
	int   m_BeatsBar;	
	
	int m_KeyColMap[12];
	
	map<int,Fl_SEvent*> m_EventMap;

	bool m_FirstUpdate;

	friend istream &operator>>(istream &s, Fl_EventMap &o);
	friend ostream &operator<<(ostream &s, Fl_EventMap &o);	
};

istream &operator>>(istream &s, Fl_EventMap &o);
ostream &operator<<(ostream &s, Fl_EventMap &o);

#endif
