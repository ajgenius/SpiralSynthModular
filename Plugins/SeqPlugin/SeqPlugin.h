/*  SpiralSound
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

#include "SpiralPlugin.h"
#include <FL/Fl_Pixmap.H>
#include "Fl_EventMap.h"

#ifndef SeqPLUGIN
#define SeqPLUGIN

const int NUM_PATTERNS = 16;

class SeqPlugin : public SpiralPlugin
{
public:
 	SeqPlugin();
	virtual ~SeqPlugin();
	
	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual SpiralGUIType *CreateGUI();
	virtual void Execute();
	virtual void StreamOut(std::ostream &s);
	virtual void StreamIn(std::istream &s);
	
	// has to be defined in the plugin	
	virtual void UpdateGUI() { Fl::check(); }
	
	void  SetEventMap(int n, Fl_EventMap* s) { m_Eventmap[n]=s; }
	Fl_EventMap* GetEventMap(int n)       { return m_Eventmap[n]; }
	void  SetUpdate(bool s)          { m_Eventmap[m_CurrentPattern]->SetUpdate(s); }
	void  SetZoom(float s)           { m_Eventmap[m_CurrentPattern]->SetZoomLevel(s); }
	void  SetNoteCut(bool s)         { m_NoteCut=s; }
	bool  GetNoteCut()               { return m_NoteCut; }	
	void  SetEndTime(float s)        { m_Eventmap[m_CurrentPattern]->SetEndTime(s); m_Length=s; }
	void  SetSpeed(float s)          { m_SpeedMod=s; }
	void  ClearAll()				 { m_Eventmap[m_CurrentPattern]->RemoveAllEvents(); }
	void  SetPattern(int s);
	int   GetCurrentPattern()		 { return m_CurrentPattern; }
	
private:
	Fl_EventMap* m_Eventmap[NUM_PATTERNS];
	
	float m_Time;
	float m_Length;
	bool  m_Loop;
	bool  m_NoteCut;
	float m_SpeedMod;		
	float m_CurrentNoteCV;
	float m_CurrentTriggerCV;
	bool  m_InNoteDown;
	int   m_InNoteID;
	float m_InNoteTime;
	int   m_CurrentPattern;
	bool  m_Triggered;
};

#endif
