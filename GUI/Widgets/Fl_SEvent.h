/*  Event Widget
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
#include <FL/Fl_Group.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_Item.H>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#ifndef SEVENT
#define SEVENT

using namespace std;

class Fl_EventMap;

class Fl_SEvent : public Fl_Group
{
public:
	enum Type{NO_TYPE,MELODY,PERCUSSION};
	enum DragMode{NONE,MOVING,RESIZING};

	Fl_SEvent(int x, int y, int w, int h, const char* label=0);
	Fl_SEvent(const Fl_SEvent &Other);
	virtual ~Fl_SEvent();
	
	virtual void draw();
	virtual int  handle(int event);
				
	void BuildMenu();
				
	int  GetID()           { return m_ID; }
	void SetID(int s)      { m_ID=s; }
	void SetChannel(int s) { m_Channel=s; }
	int  GetChannel()      { return m_Channel; }
	int  GetColour()       { return m_Colour; }
	void SetColour(int s)  { m_Colour=s; }
	void SnapX()           { if (m_GridX) x(x()-((x()-GetParentX())%m_GridX)); redraw(); }	
	void SnapY()           { if (m_GridY) y(y()-((y()-GetParentY())%m_GridY)); redraw(); }
	void SetName(string s) { m_Name=s; }
	string GetName()       { return m_Name; }
	void SetGridX(int s)   { m_GridX=s; }
	void SetGridY(int s)   { m_GridY=s; }
	void LockX()		   { m_LockX=true; }
	void LockY()		   { m_LockY=true; }
	void LockResize(bool s){ m_LockResize=s; }
	void SetResizeGrab(int s) { m_ResizeGrab=s; }
	void SetSnapGap(float s) 
		{ m_SnapGap=s; if(s) m_GridX=(int)(s*(float)m_PixelsPerSec); }
	float GetStartTime()   { return m_StartTime; }
	float GetLengthTime()  { return m_LengthTime; }
	void  SetLengthTime(float s)  { m_LengthTime=s; w((int)(m_LengthTime*m_PixelsPerSec)); redraw(); }
	int   GetGroup()       { return (y()-GetParentY())/m_GridY; }
	Type  GetType()        { return m_Type; }
	void  SetType(Type s)  { m_Type=s; }	
	void  SetPixelsPerSec(int s, bool FirstTime=false);
	int   GetParentX(); 
	int   GetParentY(); 
	void  Place(int sx, int sy) { x(sx); y(sy); }

	bool UpdateState(float Time);
	bool AtStart()         { return m_FirstUpdate; }
	bool AtEnd()           { return m_LastUpdate; }
	bool Killed()          { return m_DelMe; }
	void KillMe()          { m_DelMe=true; }
	
	int GetX() { return x(); }
	int GetY() { return y(); }
	int GetW() { return w(); }
	
private:

	Fl_Menu_Button *m_Menu; 
	
	Type m_Type;
	// whether we were active or not last tick
	bool m_LastState;
	
	// if this is the first or last update
	bool m_FirstUpdate;
	bool m_LastUpdate;

	string m_Name;
	int    m_ID;
	int    m_Colour;
	int    m_GridX;
	int    m_GridY;
	bool   m_LockX;
	bool   m_LockY;
	bool   m_LockResize;
	int    m_ResizeGrab;
	int	   m_PixelsPerSec;
	int    m_Channel;	
	
	float  m_StartTime;
	float  m_LengthTime;	
	float  m_SnapGap;
	
	DragMode m_CurrentDragMode;
	
	bool   m_DelMe;
	
	int LastButtonPushed;
};

/////////////////////////////////////////////////////////

class Fl_CircEvent : public Fl_SEvent
{
public:
	Fl_CircEvent(int x, int y, int w, int h, const char* label=0);
	Fl_CircEvent(const Fl_CircEvent &Other);
	virtual void draw();
private:
};

#endif
