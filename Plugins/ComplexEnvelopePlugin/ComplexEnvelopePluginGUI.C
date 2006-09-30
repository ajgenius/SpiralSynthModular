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
#include "ComplexEnvelopePluginGUI.h"
#include <FL/fl_draw.h>
#include <FL/fl_draw.H>

static const float TIMED_SLIDER_MAX = 3.0f;

/////////////////////////////////////////////////////////////////////////

Fl_Envelope::Fl_Handle::Fl_Handle(int x,int y, int w, int h, char *Name) :
Fl_Widget(x,y,w,h,Name),
m_DelMe(false),
m_Changed(false),
m_Coincident(false)
{
}

Fl_Envelope::Fl_Handle::~Fl_Handle()
{
}

int Fl_Envelope::Fl_Handle::handle(int event)
{
	int Mousebutton=Fl::event_button();
	if (event==FL_DRAG && Mousebutton==1)
	{
		x(Fl::event_x()-(w()/2));
		y(Fl::event_y()-(h()/2));
		
		// restrict the movement to the parent's area
		if (x()<parent()->x())                   x(parent()->x());
		if (x()>parent()->x()+parent()->w()-w()) x(parent()->x()+parent()->w()-w());
		if (y()<parent()->y())                   y(parent()->y()); 
		if (y()>parent()->y()+parent()->h()-h()) y(parent()->y()+parent()->h()-h()); 
		
		m_Changed=true;
		
		parent()->redraw();	
		redraw();			
	}
	
	if (event==FL_PUSH && Mousebutton==3)
	{
		m_DelMe=true;
	}
	
	return 1;
}

void Fl_Envelope::Fl_Handle::draw()
{
	if (m_Coincident)
	{
		fl_color(FL_BLACK);
		fl_rect(x(),y(),w()+1,h()+1);
		fl_line(x(),y(),x()+w(),y()+h());
		fl_line(x(),y()+h(),x()+w(),y());		
	}
	else
	{
		fl_color(FL_RED);
		fl_arc(	x(),y(),w()+1,h()+1,0,360);
	}
}



Fl_Envelope::Fl_Envelope(int x,int y, int w, int h, char *Name) :
Fl_Group(x,y,w,h,Name),
m_Length(1.0),
m_Bezier(true)
{
	m_Origin_x=x+10;
	m_Origin_y=y+h-10;

	// default setting				
	Fl_Handle * newhandle = new Fl_Handle(28,20,10,10,"");
	m_HandleList.push_back(newhandle);
	add(newhandle);
	
	newhandle = new Fl_Handle(14,122,10,10,"");
	m_HandleList.push_back(newhandle);
	add(newhandle);
	
	newhandle = new Fl_Handle(48,63,10,10,"");
	m_HandleList.push_back(newhandle);
	add(newhandle);
	
	newhandle = new Fl_Handle(76,91,10,10,"");
	m_HandleList.push_back(newhandle);
	add(newhandle);
	
	newhandle = new Fl_Handle(131,86,10,10,"");
	m_HandleList.push_back(newhandle);
	add(newhandle);
	
	newhandle = new Fl_Handle(194,81,10,10,"");
	m_HandleList.push_back(newhandle);
	add(newhandle);
	
	newhandle = new Fl_Handle(294,131,10,10,"");
	m_HandleList.push_back(newhandle);
	add(newhandle);
	
	newhandle = new Fl_Handle(216,124 ,10,10,"");
	m_HandleList.push_back(newhandle);
	add(newhandle);
	
	show();
}

Fl_Envelope::~Fl_Envelope()
{
	end();
}

int Fl_Envelope::handle(int event)
{
	int ret=Fl_Group::handle(event);

	// search for deleted handles
	for(list<Fl_Handle*>::iterator i = m_HandleList.begin();
		i!=m_HandleList.end(); ++i)
	{
		// if it's been modified, update the env via the callback
		if ((*i)->Changed()) do_callback();
	
		if ((*i)->Deleted())
		{
			remove(*i);
			m_HandleList.erase(i);
			break; // one at a time
		}
	}

	if (!ret)
	{
		int Mousebutton=Fl::event_button();
		if (event==FL_PUSH && Mousebutton==2)
		{
			Fl_Handle * newhandle = new 
				Fl_Handle(Fl::event_x()-5,Fl::event_y()-5,10,10,"");
			m_HandleList.push_back(newhandle);
			add(newhandle);		
		}
				
		redraw();
	}	
	
	return ret;
}

void Fl_Envelope::draw()
{
	fl_push_clip(x(),y(),w()+1,h()+1);
	fl_color(FL_GRAY);
	fl_rectf(x(),y(),w()+1,h()+1);
	
	fl_color(FL_BLACK);
	fl_rect(x(),y(),w()+1,h()+1);
	
	fl_color(FL_WHITE);
	
	int last_x=m_Origin_x, last_y=m_Origin_y, closest=INT_MAX, dist;
	list<Fl_Handle*>::iterator ClosestHandle=m_HandleList.end();

	// draw second markers
	fl_color(150,150,150);		

	for (int n=0; n<5; n++)
	{
		int p=(int)(m_Origin_x+(w()/m_Length*n));
		fl_line(p,y(),p,y()+h());
	}
	
	m_HandlePos.clear();

	m_HandlePos.push_back(Vec2(m_Origin_x,m_Origin_y));
	if (m_Bezier) AddToSpline(Vec2(m_Origin_x,m_Origin_y));
	
	for (unsigned int n=0; n<m_HandleList.size(); n++)
	{	
		// slow - search through drawing line connecting handles from left to right
		for(list<Fl_Handle*>::iterator i = m_HandleList.begin();
			i!=m_HandleList.end(); ++i)
		{
			dist=(*i)->x()-last_x;
			if (dist>-5 && dist<closest)
			{
				closest=dist;
				ClosestHandle=i;
			}			
		}
			
		if (ClosestHandle!=m_HandleList.end())
		{
			closest=INT_MAX;
			fl_color(FL_WHITE);	
			fl_line(last_x,last_y,(*ClosestHandle)->x()+5,(*ClosestHandle)->y()+5);
			
			(*ClosestHandle)->SetIsCoincident(false);
												
			last_x=(*ClosestHandle)->x()+5;
			last_y=(*ClosestHandle)->y()+5;							
			
			m_HandlePos.push_back(Vec2((*ClosestHandle)->x()+5,(*ClosestHandle)->y()+5));
			
			if (m_Bezier)
			{									
				if (!(n%3)) (*ClosestHandle)->SetIsCoincident(true);
				AddToSpline(Vec2((*ClosestHandle)->x()+5,(*ClosestHandle)->y()+5));
			}		
		}	
	}
			
	if (m_Bezier)
	{
		vector<Vec2> BezierLineList;
		CalculateBezierSpline(&BezierLineList,10);
							  
		vector<Vec2>::iterator bi=BezierLineList.begin();
		bi++;		
		fl_color(FL_BLUE);			
		for (;bi!=BezierLineList.end(); bi++)
		{					
			fl_line((int)(bi-1)->x,(int)(bi-1)->y,(int)bi->x,(int)bi->y);
		}
	}
	
	fl_color(FL_BLACK);	
	fl_line(m_Origin_x,m_Origin_y,m_Origin_x,y()+10);
	fl_line(m_Origin_x,m_Origin_y,x()+w()-10,m_Origin_y);
	
	Fl_Group::draw();
	fl_pop_clip();
}

void Fl_Envelope::Clear()
{
	for(list<Fl_Handle*>::iterator i = m_HandleList.begin();
		i!=m_HandleList.end(); ++i)
	{
		remove(*i);
	}
	
	m_HandleList.clear();
}

vector<Vec2> Fl_Envelope::GetCVList()
{
	vector<Vec2> m_CV;	
	for(vector<Vec2>::iterator i = m_HandlePos.begin();
		i!=m_HandlePos.end(); ++i)
	{
		// convert to 0->1
		Vec2 vec((i->x-m_Origin_x)/(float)(w()-m_Origin_x),
	   		     (m_Origin_y-i->y)/(float)(m_Origin_y-y()));
	
		m_CV.push_back(vec);
	}
	
	return m_CV;
}

void Fl_Envelope::SetCVList(const vector<Vec2> &CV)
{	
	Clear();
	
	for(vector<Vec2>::const_iterator i = CV.begin();
		i!=CV.end(); ++i)	
	{
		int minx=m_Origin_x, width=w()-m_Origin_x;
		int miny=m_Origin_y, height=m_Origin_y-y();		
	
		// convert from 0->1 screen coords
		Vec2 vec(minx+(i->x*width),
	   		     miny-(i->y*height));
		
		Fl_Handle *nh = new Fl_Handle((int)vec.x,(int)vec.y,10,10,"");
		add(nh);
		m_HandleList.push_back(nh);
	}
	
	redraw();
}

/////////////////////////////////////////////////////////////////////////

ComplexEnvelopePluginGUI::ComplexEnvelopePluginGUI(int w, int h,ComplexEnvelopePlugin *o,const HostInfo *Info) :
SpiralPluginGUI(w,h,o)
{	
	m_Plugin=o;
	
	m_Type = new Fl_Button(15,h-35,100,20,"Bezier Curve");
	m_Type->type(1);
	m_Type->value(1);
	m_Type->labelsize(10);	
	m_Type->callback((Fl_Callback*)cb_Type);
	
	m_Length = new Fl_Knob(w-60,h-55,40,40,"Length");
	m_Length->color(Info->GUI_COLOUR);
	m_Length->labelsize(10);	
	m_Length->maximum(5);
    m_Length->step(0.001);
    m_Length->value(1);
	m_Length->callback((Fl_Callback*)cb_Length);
	
	m_TLength = new Fl_Output(w-130,h-45,60,20,"");	
	m_TLength->value("1.0");
	
	m_Envelope = new Fl_Envelope(5,20,w-10,h-75,"");
	m_Envelope->callback((Fl_Callback*)cb_UpdateEnv);
	
	end();
}

void ComplexEnvelopePluginGUI::UpdateValues()
{
	
}

inline void ComplexEnvelopePluginGUI::cb_UpdateEnv_i(Fl_Envelope *o, void* v) 
{
	m_Plugin->CVListToEnvelope(o->GetCVList());
}
void ComplexEnvelopePluginGUI::cb_UpdateEnv(Fl_Envelope* o, void* v) 
{((ComplexEnvelopePluginGUI*)(o->parent()))->cb_UpdateEnv_i(o,v);}

inline void ComplexEnvelopePluginGUI::cb_Type_i(Fl_Button *o, void *v)
{
	m_Envelope->SetBezier(o->value());
	m_Plugin->SetBezier(o->value());
	m_Envelope->redraw();
}
void ComplexEnvelopePluginGUI::cb_Type(Fl_Button *o, void *v)
{((ComplexEnvelopePluginGUI*)(o->parent()))->cb_Type_i(o,v);}

inline void ComplexEnvelopePluginGUI::cb_Length_i(Fl_Knob *o, void *v)
{
	m_Plugin->SetLength(o->value());
	m_Envelope->SetLength(o->value());
	static char text[16];
	sprintf(text,"%f",o->value());
	m_TLength->value(text);
	m_Envelope->redraw();
}
void ComplexEnvelopePluginGUI::cb_Length(Fl_Knob *o, void *v)
{((ComplexEnvelopePluginGUI*)(o->parent()))->cb_Length_i(o,v);}
