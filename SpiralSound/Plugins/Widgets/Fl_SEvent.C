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

#include "Fl_SEvent.h"
#include "Fl_EventMap.h"
#include <Fl/fl_draw.H>
#include <Fl/Fl_Window.H>
#include <FL/x.H>
#include <stdio.h>

static const int MELODY_WIDGET_COL = 142;
static const int PERCUSSION_WIDGET_COL = 140;
static const int EVENT_RESIZE_GRAB = 10;

Fl_SEvent::Fl_SEvent(int x, int y, int w, int h, const char* label) :
Fl_Group(x,y,w,h,""),
m_Type(NO_TYPE),
m_LastState(false),
m_FirstUpdate(false),
m_LastUpdate(false),
m_Name(""),
m_ID(-1),
m_Colour(PERCUSSION_WIDGET_COL),
m_GridX(0),
m_GridY(0),
m_LockX(false),
m_LockY(false),
m_LockResize(true),
m_ResizeGrab(8),
m_PixelsPerSec(100),
m_Channel(0),
m_StartTime(0.0f),
m_LengthTime(0.0f),
m_SnapGap(0.0f),
m_CurrentDragMode(NONE),
m_DelMe(false)
{
    Fl_Group::label(m_Name.c_str());
	labelsize(8);
}

void Fl_SEvent::BuildMenu()
{	
	Fl_EventMap::EventCallbacks *cb =&((Fl_EventMap*)parent())->m_Callbacks;

	m_Menu = new Fl_Menu_Button(x(),y(),w(),h(),"");
	m_Menu->type(Fl_Menu_Button::POPUP3);
	m_Menu->textsize(8);
		
	if (cb->cb_CopyEvent) m_Menu->add("copy", 0, cb->cb_CopyEvent,this);
	if (cb->cb_CloneEvent) m_Menu->add("clone", 0, cb->cb_CloneEvent,this);
	if (cb->cb_InstanceEvent) m_Menu->add("instance", 0, cb->cb_InstanceEvent,this);	
	if (cb->cb_DelEvent) m_Menu->add("delete", 0, cb->cb_DelEvent,this);
	if (cb->cb_RenameEvent) m_Menu->add("rename", 0, cb->cb_RenameEvent,this);
	if (cb->cb_Recolour) m_Menu->add("colour", 0, cb->cb_Recolour,this);
	if (cb->cb_EditEvent) m_Menu->add("edit", 0, cb->cb_EditEvent,this);
	add(m_Menu);
}


Fl_SEvent::Fl_SEvent(const Fl_SEvent &Other) :
Fl_Group(Other.x(),Other.y(),Other.w(),Other.h(),Other.label()),
m_Type(Other.m_Type),
m_LastState(Other.m_LastState),
m_FirstUpdate(Other.m_FirstUpdate),
m_LastUpdate(Other.m_LastUpdate),
m_Name(Other.m_Name),
m_ID(Other.m_ID),
m_Colour(Other.m_Colour),
m_GridX(Other.m_GridX),
m_GridY(Other.m_GridY),
m_LockX(Other.m_LockX),
m_LockY(Other.m_LockY),
m_LockResize(Other.m_LockResize),
m_ResizeGrab(Other.m_ResizeGrab),
m_PixelsPerSec(Other.m_PixelsPerSec),
m_Channel(Other.m_Channel),
m_StartTime(Other.m_StartTime),
m_LengthTime(Other.m_LengthTime),
m_SnapGap(Other.m_SnapGap),
m_CurrentDragMode(Other.m_CurrentDragMode),
m_DelMe(false),
LastButtonPushed(0)
{
}

Fl_SEvent::~Fl_SEvent()
{
}

bool Fl_SEvent::UpdateState(float Time)   
{ 
	m_FirstUpdate=false;
	m_LastUpdate=false;
	
	bool State=(Time>m_StartTime && Time<m_StartTime+m_LengthTime); 
	
	if (State && !m_LastState) m_FirstUpdate=true;
	if (!State && m_LastState) m_LastUpdate=true; 
	
	m_LastState=State;
	
	return State;
}
		
void Fl_SEvent::draw()
{
	Fl_Group::draw();
	// check if clipped
	//if (fl_clip()) return;

 	// clamp width 
	if (w()<5) w(5);

	Fl_EventMap* P=(Fl_EventMap*)(parent());

	if (P->GetType()==Fl_EventMap::MELODY_MAP)
	{			
		int Col=FL_BLACK;
		if (parent()) Col=((Fl_EventMap*)(parent()))->GetKeyColMap()[GetGroup()%12];
		fl_color(Col);
		fl_rectf(x()+1,y()+1,w()-1,h()-1);
		
		// draw shadow
		fl_color(FL_BLACK);
		fl_line(x()+w(),y()+1,x()+w(),y()+h());
		fl_line(x()+w(),y()+h(),x()+1,y()+h());
	}
	else
	{	
		fl_color(m_Colour);
		
		fl_rectf(x()+1,y()+1,w()-1,h()-1);
	
		// draw shadow
		fl_color(FL_BLACK);
		fl_line(x()+w(),y()+1,x()+w(),y()+h());
		fl_line(x()+w(),y()+h(),x()+1,y()+h());
		
		fl_push_clip(x()+1,y()+1,w()-1,h()-1);
		fl_font(fl_font(), 8);
		fl_draw(m_Name.c_str(),x()+2,y()+h()-2);
		fl_pop_clip();
	}
	
	
	// todo: sample rendering	
}

int Fl_SEvent::handle(int  event)
{
	int mx=Fl::event_x();
	int my=Fl::event_y();				

	m_Menu->resize(x(),y(),w(),h());

	Fl_EventMap::EventCallbacks *cb =&((Fl_EventMap*)parent())->m_Callbacks;

	static int offsx,offsy;
	
	if (Fl::event_button()==1 && event==FL_PUSH && Fl::event_clicks()==1) 
	{
		if (cb->cb_EventDoubleClicked!=NULL)
		{
			cb->cb_EventDoubleClicked(this,NULL);
		}
	}
	
	switch (event) 
	{		
  		case FL_PUSH:
			LastButtonPushed=Fl::event_button();					
	
	        offsx=mx-x();
	        offsy=my-y();
			
			/*if (Fl::event_key(FL_BackSpace)) 
			{				
				Fl_EventMap *p = (Fl_EventMap*)parent();
				if (p && p->GetType()!=Fl_EventMap::ARRANGE_MAP) 
				{
					if (cb->cb_DelEvent!=NULL) cb->cb_DelEvent(this,NULL);
					m_DelMe=true;
				}
			}*/	
			
			if (LastButtonPushed==1) 
			{					
				// if the last EVENT_RESIZE_GRAB pixels 
				// have been grabbed, resize.
				if (offsx>w()-EVENT_RESIZE_GRAB) m_CurrentDragMode=RESIZING;
				else m_CurrentDragMode=MOVING;
			}
						
			// fall through
			case FL_DRAG:
    		{				
				if (LastButtonPushed==1 || LastButtonPushed==3) 
				{
					if(m_CurrentDragMode==RESIZING)
					{
						w(mx-x()+(m_ResizeGrab/2));
						//m_Menu->size(w(),h());
						
						m_LengthTime=w()/(float)m_PixelsPerSec;	
					}
					
					if(m_CurrentDragMode==MOVING)
					{
                        x(mx-offsx);
                        y(my-offsy);
						m_Menu->position(x(),y());
						
						m_StartTime=(x()-GetParentX())/(float)m_PixelsPerSec;
					}										
					
					my=y();
					SnapY();
					
					if (cb->cb_MoveEvent!=NULL) cb->cb_MoveEvent(this,NULL);
					parent()->redraw();
				}
			} 
			break;
	
		case FL_RELEASE:
			{
				m_CurrentDragMode=NONE;				
			}
			break;
	}
	
	Fl_Group::handle(event);
	
	return 1;
}

void Fl_SEvent::SetPixelsPerSec(int s, bool FirstTime)
{
	m_PixelsPerSec=s;
	
	if (FirstTime)
	{
		m_StartTime=(x()-GetParentX())/(float)m_PixelsPerSec;
		m_LengthTime=w()/(float)m_PixelsPerSec;	
	}
	else
	{
		// convert back to new value
		
		//get relative x
		int rel_x=x()-GetParentX();
		
		rel_x=(int)(m_StartTime*m_PixelsPerSec);
		x(rel_x+GetParentX());
		
		w((int)(m_LengthTime*m_PixelsPerSec));
		m_GridX=(int)(m_SnapGap*(float)m_PixelsPerSec);
	}
}

int Fl_SEvent::GetParentX()
{
	Fl_EventMap* P=(Fl_EventMap*)(parent());
	if (P)
	{
		return P->GetX();
	}
	else
	{
		return 0;
	}
}

int Fl_SEvent::GetParentY()
{
	Fl_EventMap* P=(Fl_EventMap*)(parent());
	if (P)
	{
		return P->GetY();
	}
	else
	{
		return 0;
	}
}

////////////////////////////////////////////////////////////////////

Fl_CircEvent::Fl_CircEvent(int x, int y, int w, int h, const char* label) :
Fl_SEvent(x,y,12,h,label)
{
}

Fl_CircEvent::Fl_CircEvent(const Fl_CircEvent &Other) :
Fl_SEvent(Other)
{
}
	
void Fl_CircEvent::draw()
{
	// hackish way to override the zoom scale of the width
	w(12);

	int Col=0;
	if (parent()) Col=((Fl_EventMap*)(parent()))->GetKeyColMap()[GetGroup()%12];
	fl_color(Col);
	
	fl_rectf(x(),y()+1,12,12);
	
	fl_color(FL_BLACK);
	fl_line(x()+1,y()+13, x()+13,y()+13);
	fl_line(x()+12,y()+13, x()+12,y()+1);
	
//	fl_pie(x(),y(),12,12,0,360);
}
