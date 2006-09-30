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
#include <list>

#ifndef SequencerPLUGIN
#define SequencerPLUGIN

const int NUM_PATTERNS = 8;
const int NUM_CHANNELS = 8;

//////////////////////////////////////////////////////////////////////////
// base sequencing stuff

class Note
{
public:
	Note(float t, float l, int n, float v);
	virtual ~Note();
	
	float Time;
	float Length;
	int   MidiNote;
	float Vol;

	friend std::istream &operator>>(std::istream &s, Note &o);
	friend std::ostream &operator<<(std::ostream &s, Note &o);
};

std::istream &operator>>(std::istream &s, Note &o);
std::ostream &operator<<(std::ostream &s, Note &o);

class Pattern
{
public:
	Pattern();
	virtual ~Pattern();
	
	void  Copy(const Pattern *o);
	void  AddNote(int ID, float t, float l, int n, float v);
	void  RemoveNote(int ID);
	Note *GetNote(int ID);
	int   GetNoteCount() { return m_NoteMap.size(); }
	
	map<int,Note> m_NoteMap;
	
	friend std::istream &operator>>(std::istream &s, Pattern &o);
	friend std::ostream &operator<<(std::ostream &s, Pattern &o);
};

std::istream &operator>>(std::istream &s, Pattern &o);
std::ostream &operator<<(std::ostream &s, Pattern &o);
	
class Sequence
{
public:	
	Sequence(float st=0, int pat=0);
	virtual ~Sequence();

	void   SetStartTime(float t) { m_StartTime=t; }
	float  GetStartTime()        { return m_StartTime; }
	void   SetPatternID(int p)   { m_Pattern=p; }
	int    GetPatternID()        { return m_Pattern; }
	void   SetName(string s)     { m_Name=s; }
	string GetName()             { return m_Name; }
	void   SetColour(int s)      { m_Colour=s; }
	int    GetColour()           { return m_Colour; }
	void   SetYPos(int s)        { m_YPos=s; }
	int    GetYPos()             { return m_YPos; }
	void   SetLength(float t)    { m_Length=t; }
	float  GetLength()           { return m_Length; }
	void   SetChannel(int s)     { m_Channel=s; }
	int    GetChannel()          { return m_Channel; }

private:
	float m_StartTime;
	int   m_Pattern;
	string m_Name;
	int    m_Colour;
	int    m_YPos;
	float  m_Length;
	int    m_Channel;	

	friend std::istream &operator>>(std::istream &s, Sequence &o);
	friend std::ostream &operator<<(std::ostream &s, Sequence &o);
};
	
std::istream &operator>>(std::istream &s, Sequence &o);
std::ostream &operator<<(std::ostream &s, Sequence &o);

class Track
{
public:
	Track();
	virtual  ~Track();

	void      AddSequence(int ID);
	void      CloneSequence(int ID, int nID);
	void      CopySequence(int ID, int nID);
	void      InstanceSequence(int ID);
	void      RemoveSequence(int ID);
	Sequence *GetSequence(int ID);
	
	void      AddNote(int ID, int Sequence, float t, float l, int n, float v);
	void      RemoveNote(int ID, int Sequence);
	void      ChangeNote(int ID, int Sequence, float t, float l, int n, float v);
	void      ReadTrack(float t, int channel, vector<Note> &NoteVec);	
	Pattern  *GetPattern(int ID);
	
	map<int,Sequence>  *GetSequenceMap() { return &m_SequenceMap; }
	
private:
	void      AddPattern(int ID);
	void      RemovePattern(int ID);
	
	std::map<int,Pattern>  m_PatternMap;
	std::map<int,Sequence> m_SequenceMap;
	
	int m_NextPatternID;
	
	friend std::istream &operator>>(std::istream &s, Track &o);
	friend std::ostream &operator<<(std::ostream &s, Track &o);
};

std::istream &operator>>(std::istream &s, Track &o);
std::ostream &operator<<(std::ostream &s, Track &o);

///////////////////////////////////////////////////////////////////////////

class SequencerPlugin : public SpiralPlugin
{
public:
 	SequencerPlugin();
	virtual ~SequencerPlugin();
	
	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual SpiralGUIType *CreateGUI();
	virtual void Execute();
	virtual void ExecuteCommands();
	virtual void StreamOut(ostream &s);
	virtual void StreamIn(istream &s);
		
	bool  GetNoteCut()               { return m_NoteCut; }	
	void  ClearAll()				 { /*m_Eventmap[m_CurrentPattern]->RemoveAllEvents();*/ }
	int   GetCurrentPattern()		 { return m_CurrentPattern; }
	Track *GetTrack()                { return &m_Track; }
	
	enum GUICommands {NONE,NEW_NOTE,REM_NOTE,CHG_NOTE,NEW_SEQ,CHG_SEQ,COPY_SEQ,
					  INST_SEQ,CLONE_SEQ,REM_SEQ,GET_PATTERN};
	
	struct GUIArgs
	{
		int Num,Num2;
		int Sequence;
		float t,l,v;
		int n;
		char Name[256];
		int  Channel;
	};
	
private:

	GUIArgs m_GUIArgs;

	Track m_Track;

	int m_TransferPattern;
	//int m_TransferNote;
	map<int,Note>::iterator m_TransferNote;
	int m_TransferCount;
	Note m_Transfer;
	
	void  SetEventMap(int n, Fl_EventMap* s) { /*m_Eventmap[n]=s;*/ }
	void  SetUpdate(bool s)          { /*m_Eventmap[m_CurrentPattern]->SetUpdate(s);*/ }
	void  SetZoom(float s)           { /*m_Eventmap[m_CurrentPattern]->SetZoomLevel(s);*/ }
	void  SetNoteCut(bool s)         { m_NoteCut=s; }
	//void  SetEndTime(float s)        { /*m_Eventmap[m_CurrentPattern]->SetEndTime(s);*/ m_Length=s; }
	void  SetSpeed(float s)          { m_SpeedMod=s; }
	void  SetPattern(int s);
	
	float m_Time;
	int   m_Length; // in bars
	float m_BarLength;
	int   m_BeatsPerBar;
	float m_NextBeatTime;
	float m_BeatLevel;
	int   m_BeatCount;	
	int   m_BarCount;	
	bool  m_Loop;
	bool  m_NoteCut;
	float m_SpeedMod;		

	float m_CurrentNoteCV[NUM_CHANNELS];
	float m_CurrentTriggerCV[NUM_CHANNELS];

	bool  m_InNoteDown;
	int   m_InNoteID;
	float m_InNoteTime;
	int   m_CurrentPattern;
	bool  m_Triggered;
};

#endif
