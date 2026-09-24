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
#include <FL/Fl_Output.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Output.H>
#include <list>

#include "ComplexEnvelopePlugin.h"
#include "SpiralPluginGUI.h"
#include "Fl_Knob.H"
#include "Bezier.h"

#ifndef EnvelopeGUI
#define EnvelopeGUI

class Fl_Envelope : public Fl_Group
{
public:
	Fl_Envelope(int x,int y, int w, int h, char *Name);
	~Fl_Envelope();
	
	virtual int handle(int event);
	virtual void draw();
	void Clear();
	void SetBezier(bool s) { m_Bezier=s; }
	void SetLength(float s) { m_Length=s; }
	vector<Vec2> GetCVList();
	void SetCVList(const vector<Vec2> &CV);
		
private:
	
	class Fl_Handle : public Fl_Widget
	{
	public:
		Fl_Handle(int x,int y, int w, int h, char *Name);
		~Fl_Handle();
		virtual int handle(int event);
		virtual void draw();
		bool Deleted() { return m_DelMe; }
		void DelMe()   { m_DelMe=true; }
		void SetIsCoincident(bool s) { m_Coincident=s; }		
		bool Changed() { bool t=m_Changed; m_Changed=false; return t; }
		
	private:
		bool m_DelMe;
		bool m_Changed;
		bool m_Coincident;
	};

	float m_Length;
	float m_Time;
	bool  m_Bezier;
	
	list<Fl_Handle*> m_HandleList;
	vector<Vec2>     m_HandlePos;
	int m_Origin_x;
	int m_Origin_y;		
};


class ComplexEnvelopePluginGUI : public SpiralPluginGUI
{
public:
	ComplexEnvelopePluginGUI(int w, int h, ComplexEnvelopePlugin *o,const HostInfo *Info);
	
	virtual void UpdateValues();
	virtual SpiralPlugin* GetPlugin() { return m_Plugin; }
	
	void SetCVList(const vector<Vec2> &CV) { m_Envelope->SetCVList(CV); }
			
	ComplexEnvelopePlugin *m_Plugin;	
private:
	
	Fl_Envelope *m_Envelope;
	Fl_Button   *m_Type;
	Fl_Knob     *m_Length;
	Fl_Output   *m_TLength;
	
	//// Callbacks ////
	
	inline void cb_Type_i(Fl_Button *o, void *v);
	static void cb_Type(Fl_Button *o, void *v);
	inline void cb_Length_i(Fl_Knob *o, void *v);
	static void cb_Length(Fl_Knob *o, void *v);
	inline void cb_UpdateEnv_i(Fl_Envelope *o, void*v); 
	static void cb_UpdateEnv(Fl_Envelope* o, void* v);
};

#endif
