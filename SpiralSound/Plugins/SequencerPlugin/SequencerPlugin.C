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
#include "SequencerPlugin.h"
#include "SequencerPluginGUI.h"
#include <Fl/Fl_Button.H>
#include "SpiralIcon.xpm"
#include "../../NoteTable.h"

using namespace std;

// for note on's
static const float TRIGGER_LEV=0.1;

//////////////////////////////////////////////////////////////////////////
// base sequencing stuff

Note::Note(float t=0, float l=0.1f, int n=0, float v=1.0f)
{
	Time = t;
	Length = l;
	MidiNote = n;
	Vol  = v;
}

Note::~Note()
{
}

istream &operator>>(istream &s, Note &o)
{
	int version=1;
	string dummy;
	s>>version>>dummy>>o.Time>>o.Length>>o.MidiNote>>o.Vol;

	return s;
}

ostream &operator<<(ostream &s, Note &o)
{
	int version=1;
	s<<version<<" note "<<o.Time<<" "<<o.Length<<" "<<o.MidiNote<<" "<<o.Vol<<" ";

	return s;
}

/////////////////////////////////////////////////////////////////////////

Pattern::Pattern()
{
}

Pattern::~Pattern()
{
}

void  Pattern::Copy(const Pattern *o)
{
	// can't do this as we don't want to preserve the id numbers
	//m_NoteMap=o->m_NoteMap;

	int c=0;
	for (map<int,Note>::const_iterator i = o->m_NoteMap.begin();
		 i!=o->m_NoteMap.end(); i++)
	{
		m_NoteMap[c++]=i->second;
	}
}

void  Pattern::AddNote(int ID, float t, float l, int n, float v)
{
	map<int,Note>::iterator i = m_NoteMap.find(ID);
	if (i != m_NoteMap.end())
	{
		cerr<<"duplicate note "<<ID<<" not added"<<endl;
		return;
	}
	Note newnote(t,l,n,v);

	m_NoteMap[ID]=newnote;
}

void  Pattern::RemoveNote(int ID)
{
	map<int,Note>::iterator i = m_NoteMap.find(ID);
	if (i == m_NoteMap.end())
	{
		cerr<<"couldn't find note "<<ID<<" not removed"<<endl;
		return;
	}

	m_NoteMap.erase(i);
}

Note *Pattern::GetNote(int ID)
{
	map<int,Note>::iterator i = m_NoteMap.find(ID);
	if (i == m_NoteMap.end())
	{
		cerr<<"couldn't find note "<<ID<<endl;
		return NULL;
	}

	return &i->second;
}

istream &operator>>(istream &s, Pattern &o)
{
	int version=1,Num;
	string dummy;
	int id=0;
	s>>version>>dummy>>Num;
	for (int n=0; n<Num; n++)
	{
		Note t;
		s>>t;
		o.m_NoteMap[id++]=t;
	}

	return s;
}

ostream &operator<<(ostream &s, Pattern &o)
{
	int version=1;
	s<<version<<" pattern "<<o.m_NoteMap.size()<<" ";

	for (map<int,Note>::iterator i = o.m_NoteMap.begin();
		 i!=o.m_NoteMap.end(); i++)
	{
		s<<i->second;
	}
	s<<endl;

	return s;
}

//////////////////////////////////////////////////////////////////////////

Sequence::Sequence(float st, int pat)
{
	m_StartTime=st;
	m_Pattern=pat;
}

Sequence::~Sequence()
{
}

istream &operator>>(istream &s, Sequence &o)
{
	int version=1;
	string dummy;
	s>>version>>dummy>>o.m_StartTime>>o.m_Pattern>>o.m_Colour>>o.m_YPos>>o.m_Length>>o.m_Channel;
	
	char Buf[4096];
	int size;
	s>>size;
	s.ignore(1);
	s.get(Buf,size+1);
	o.m_Name=Buf;
	
	return s;
}

ostream &operator<<(ostream &s, Sequence &o)
{	
	int version=1;
	s<<version<<" sequence"<<" "<<o.m_StartTime<<" "<<o.m_Pattern<<" "<<o.m_Colour<<" "<<o.m_YPos<<" "<<o.m_Length<<" "<<o.m_Channel<<" ";
	s<<o.m_Name.size()<<" "<<o.m_Name<<endl;
	
	return s;
}	

//////////////////////////////////////////////////////////////////////////

Track::Track()
{
	m_NextPatternID=0;
}

Track::~Track()
{
}

void Track::AddSequence(int ID)
{
	map<int,Sequence>::iterator i = m_SequenceMap.find(ID);
	if (i != m_SequenceMap.end())
	{
		cerr<<"duplicate sequence "<<ID<<" not added"<<endl;
		return;
	}
		
	Sequence newsequence;
	AddPattern(m_NextPatternID);
	newsequence.SetPatternID(m_NextPatternID);
	newsequence.SetColour(0);
	newsequence.SetName("noname");
	newsequence.SetLength(10.0f);
	newsequence.SetYPos(1);	
	newsequence.SetChannel(0);
	m_NextPatternID++;
	
	m_SequenceMap[ID]=newsequence;
}

// duplicate the sequence, but reference the same pattern
void Track::CloneSequence(int ID, int nID)
{
	AddSequence(nID);
	Sequence *s=GetSequence(nID);
	s->SetColour(GetSequence(ID)->GetColour());
	s->SetName(GetSequence(ID)->GetName());
	s->SetLength(GetSequence(ID)->GetLength());
	s->SetYPos(GetSequence(ID)->GetYPos());	
	s->SetPatternID(GetSequence(ID)->GetPatternID());
	s->SetStartTime(GetSequence(ID)->GetStartTime());	
	s->SetChannel(GetSequence(ID)->GetChannel());
}

// duplicate the sequence, and copy the pattern to it's own version
void Track::CopySequence(int ID, int nID)
{
	AddSequence(nID);
	Sequence *s=GetSequence(nID);
	s->SetColour(GetSequence(ID)->GetColour());
	s->SetName(GetSequence(ID)->GetName());
	s->SetLength(GetSequence(ID)->GetLength());
	s->SetYPos(GetSequence(ID)->GetYPos());
	s->SetStartTime(GetSequence(ID)->GetStartTime());	
	s->SetChannel(GetSequence(ID)->GetChannel());	
	AddPattern(m_NextPatternID);
	GetPattern(m_NextPatternID)->Copy(GetPattern(GetSequence(ID)->GetPatternID()));	
	GetSequence(nID)->SetPatternID(m_NextPatternID);
	m_NextPatternID++;
}

// copy the sequences pattern, so it's got it's own version
void Track::InstanceSequence(int ID)
{
	AddPattern(m_NextPatternID);
	GetPattern(m_NextPatternID)->Copy(GetPattern(GetSequence(ID)->GetPatternID()));	
	GetSequence(ID)->SetPatternID(m_NextPatternID);
	m_NextPatternID++;
}

void Track::RemoveSequence(int ID)
{
	map<int,Sequence>::iterator i = m_SequenceMap.find(ID);
	if (i == m_SequenceMap.end())
	{
		cerr<<"couldn't find sequence "<<ID<<" not removed"<<endl;
		return;
	}
	
	// todo - refcount the patterns
	
	m_SequenceMap.erase(i);
}

Sequence *Track::GetSequence(int ID)
{
	map<int, Sequence>::iterator i = m_SequenceMap.find(ID);
	if (i == m_SequenceMap.end())
	{
		cerr<<"couldn't find sequence "<<ID<<endl;
		return NULL;
	}
	
	return &i->second;
}

void Track::AddNote(int ID, int Sequence, float t, float l, int n, float v)
{
	GetPattern(GetSequence(Sequence)->GetPatternID())->AddNote(ID,t,l,n,v);
}

void Track::RemoveNote(int ID, int Sequence)
{
	GetPattern(GetSequence(Sequence)->GetPatternID())->RemoveNote(ID);
}

void Track::ChangeNote(int ID, int Sequence, float t, float l, int n, float v)
{
	Note *note = GetPattern(GetSequence(Sequence)->GetPatternID())->GetNote(ID);
	note->Time = t;
	note->Length = l;
	note->MidiNote = n;
	note->Vol  = v;
}
	
void Track::ReadTrack(float t, int channel, vector<Note> &NoteVec)
{
	// for every sequence
	for (map<int, Sequence>::iterator i = m_SequenceMap.begin();
		 i!=m_SequenceMap.end(); i++)
	{
		if (i->second.GetChannel()==channel)
		{
			Pattern *p=GetPattern(i->second.GetPatternID());
			float SeqTime=i->second.GetStartTime();
		
			// for every note in the pattern
			for (map<int, Note>::iterator n = p->m_NoteMap.begin();
			 	 n!=p->m_NoteMap.end(); n++)
			{
				if (n->second.Time+SeqTime<t && n->second.Time+n->second.Length+SeqTime>t)
				{
					NoteVec.push_back(n->second);
				}
			}
		}
	}
}

//// private //////

void Track::AddPattern(int ID)
{
	map<int,Pattern>::iterator i = m_PatternMap.find(ID);
	if (i != m_PatternMap.end())
	{
		cerr<<"duplicate pattern "<<ID<<" not added"<<endl;
		return;
	}
	Pattern newpattern;
	m_PatternMap[ID]=newpattern;
}

void Track::RemovePattern(int ID)
{
	map<int,Pattern>::iterator i = m_PatternMap.find(ID);
	if (i == m_PatternMap.end())
	{
		cerr<<"couldn't find pattern "<<ID<<" not removed"<<endl;
		return;
	}
	
	m_PatternMap.erase(i);
}

Pattern *Track::GetPattern(int ID)
{
	map<int,Pattern>::iterator i = m_PatternMap.find(ID);
	if (i == m_PatternMap.end())
	{
		cerr<<"couldn't find pattern "<<ID<<endl;
		return NULL;
	}
	
	return &i->second;
}

istream &operator>>(istream &s, Track &o)
{
	int version=1,Num,id=0;
	string dummy;
	s>>version>>dummy;
	
	s>>Num;
	for (int n=0; n<Num; n++)
	{
		Pattern t;
		s>>t;
		o.m_PatternMap[id++]=t;
	}
	s>>Num;
	id=0;
	for (int n=0; n<Num; n++)
	{
		Sequence t;
		s>>t;
		o.m_SequenceMap[id++]=t;
		
		cerr<<"Loaded sequence "<<t.GetName()<<endl;
	}
	s>>o.m_NextPatternID;
	o.m_NextPatternID=o.m_PatternMap.size();
	return s;
}

ostream &operator<<(ostream &s, Track &o)
{	
	int version=1;
	
	s<<version<<" "<<"track"<<" ";
	s<<o.m_PatternMap.size()<<endl;	
	for (map<int,Pattern>::iterator i = o.m_PatternMap.begin();
		 i!=o.m_PatternMap.end(); i++)
	{
		s<<i->second;
	}	
	
	s<<o.m_SequenceMap.size()<<endl;	
	for (map<int,Sequence>::iterator i = o.m_SequenceMap.begin();
		 i!=o.m_SequenceMap.end(); i++)
	{
		s<<i->second;
	}
	s<<o.m_NextPatternID<<" "<<endl;
	return s;
}	
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

extern "C" {
SpiralPlugin* SpiralPlugin_CreateInstance()
{
	return new SequencerPlugin;
}

char** SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}

int SpiralPlugin_GetID()
{
	return 0x0011;
}

string SpiralPlugin_GetGroupName()
{
	return "Sequencing";
}
}

///////////////////////////////////////////////////////

SequencerPlugin::SequencerPlugin() :
m_TransferPattern(-1),
m_Time(0.0f),
m_Length(4),
m_BarLength(1.0f),
m_BeatsPerBar(4),
m_NextBeatTime(0.0f),
m_BeatLevel(-1.0f),
m_BeatCount(0),
m_BarCount(0),
m_SpeedMod(1.0f),
m_InNoteDown(false),
m_InNoteID(0),
m_CurrentPattern(0),
m_Triggered(false)
{
	m_Version=1;
	
	m_PluginInfo.Name="Sequencer";
	m_PluginInfo.Width=540;
	m_PluginInfo.Height=290;
	m_PluginInfo.NumInputs=4;
	m_PluginInfo.NumOutputs=17;		
	m_PluginInfo.PortTips.push_back("Play Trigger");	
	m_PluginInfo.PortTips.push_back("Speed CV");	
	m_PluginInfo.PortTips.push_back("Input Pitch CV");	
	m_PluginInfo.PortTips.push_back("Input Trigger CV");	
	m_PluginInfo.PortTips.push_back("Channel 0 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 0 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 1 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 1 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 2 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 2 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 3 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 3 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 4 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 4 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 5 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 5 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 6 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 6 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 7 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 7 Trigger");	
	/*m_PluginInfo.PortTips.push_back("Channel 8 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 8 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 9 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 9 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 10 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 10 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 11 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 11 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 12 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 12 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 13 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 13 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 14 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 14 Trigger");	
	m_PluginInfo.PortTips.push_back("Channel 15 Pitch");	
	m_PluginInfo.PortTips.push_back("Channel 15 Trigger");*/		
	m_PluginInfo.PortTips.push_back("Beat");	
	
	m_AudioCH->Register("ID",&m_GUIArgs.Num);
	m_AudioCH->Register("ID2",&m_GUIArgs.Num2);
	m_AudioCH->Register("Channel",&m_GUIArgs.Channel);
	m_AudioCH->Register("Sequence",&m_GUIArgs.Sequence);
	m_AudioCH->Register("Time",&m_GUIArgs.t);
	m_AudioCH->Register("Length",&m_GUIArgs.l);
	m_AudioCH->Register("Vol",&m_GUIArgs.v);
	m_AudioCH->Register("Note",&m_GUIArgs.n);
	m_AudioCH->Register("CurrentTime",&m_Time,ChannelHandler::OUTPUT);
	m_AudioCH->Register("TotalLength",&m_Length);
	m_AudioCH->Register("BeatsPerBar",&m_BeatsPerBar);
	m_AudioCH->Register("BarLength",&m_BarLength);
	m_AudioCH->RegisterData("Name",ChannelHandler::INPUT,m_GUIArgs.Name,sizeof(m_GUIArgs.Name));
	
	// use OUTPUT_REQUEST for these, as we don't have to copy them constantly (and the gui can request them)
	m_AudioCH->Register("TransCount",&m_TransferCount,ChannelHandler::OUTPUT_REQUEST);
	m_AudioCH->RegisterData("TransNote",ChannelHandler::OUTPUT_REQUEST,&m_Transfer,sizeof(m_Transfer));
}

SequencerPlugin::~SequencerPlugin()
{
}

PluginInfo &SequencerPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);
}

SpiralGUIType *SequencerPlugin::CreateGUI()
{
	return new SequencerPluginGUI(m_PluginInfo.Width,
								  	    m_PluginInfo.Height,
										this,m_AudioCH,m_HostInfo);
}

void SequencerPlugin::SetPattern(int s)          
{ 
	//m_Eventmap[m_CurrentPattern]->hide();
	m_CurrentPattern=s;
	//m_Eventmap[m_CurrentPattern]->show();
}


void SequencerPlugin::Execute()
{	
	float Speed;
	
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		if (GetInputPitch(0,n)>0) 		
		{						
			if (!m_Triggered)
			{
				float Freq=GetInputPitch(0,n);
		
				// Notes 0 to 16 trigger patterns 0 to 16
				// No other notes catered for
				for (int i=0; i<NUM_PATTERNS; i++)
				{
					if (feq(Freq,NoteTable[i],0.01f))
					{
						SetPattern(i);
						break;
					}
				}
				
				m_Time=0;
				m_Triggered=true;
			}
		}
		else
		{
			m_Triggered=false;
		}

		
		if (InputExists(1))
			Speed =(1.1025f/m_HostInfo->SAMPLERATE) * (GetInput(1,n)+1.0f);
		else
			Speed =1.1025f/m_HostInfo->SAMPLERATE;
		
		if (!m_InNoteDown)
		{
			// Check trigger
			if (GetInput(3,n)>TRIGGER_LEV)
			{
				m_InNoteDown=true;
				
				float Freq=GetInputPitch(2,n);

				int NoteNum=0;				
				for (int i=0; i<131; i++)
				{
					if (feq(Freq,NoteTable[i],0.01f))
					{				
						NoteNum=i;
						break;
					}
				}
				
				/*cerr<<"note recieved ="<<NoteNum<<" f="<<Freq
				<<" t-1="<<NoteTable[NoteNum-1]
				<<" t="<<NoteTable[NoteNum]
				<<" t+1="<<NoteTable[NoteNum+1]<<endl;
				*/
				//m_InNoteID=m_Eventmap[m_CurrentPattern]->AddEventTime(m_Time, NoteNum, 0.5, Fl_SEvent::MELODY, false);
				
				m_InNoteTime=m_Time;
			}			
		}
		else
		{
			// Check trigger
			if (GetInput(3,n)<TRIGGER_LEV)
			{
				//m_Eventmap[m_CurrentPattern]->SetEventLength(m_InNoteID, m_Time-m_InNoteTime);
				m_InNoteDown=false;
			}		
		}		
		
		for (int channel=0; channel<NUM_CHANNELS; channel++)
		{
			// Get the notes from the map		
			vector<Note> NoteVec;
			m_Track.ReadTrack(m_Time,channel,NoteVec);
			
			m_CurrentTriggerCV[channel]=0;
			//m_CurrentNoteCV=0;
			
			// play all the notes found
			for (vector<Note>::iterator i=NoteVec.begin();
			     i!=NoteVec.end(); i++)
			{
				//cerr<<"time = "<<m_Time<<endl;
				//cerr<<"found note "<<i->Time<<" "<<i->Length<<endl;
								
				m_CurrentNoteCV[channel]=NoteTable[i->MidiNote];
				m_CurrentTriggerCV[channel]=1;			
			}
			SetOutputPitch(channel*2,n,m_CurrentNoteCV[channel]);
			SetOutput(channel*2+1,n,m_CurrentTriggerCV[channel]);
		}
	
		m_Time+=Speed*m_SpeedMod;
		
		// deal with the beat calculation
		if (m_Time>m_NextBeatTime)
		{
			float BeatTime=m_BarLength/m_BeatsPerBar;
			m_NextBeatTime=m_Time+BeatTime;
			if (m_BeatLevel!=1.0f) m_BeatLevel=1.0f;
			else m_BeatLevel=-1.0f;
			
			m_BeatCount++;	
		}
		SetOutput(NUM_CHANNELS*2,n,m_BeatLevel);
		
		if (m_BeatCount>=m_BeatsPerBar) 
		{
			m_BeatCount=0;
			m_BarCount++;
		}
		
		if (m_BarCount>=m_Length)
		{
			m_Time=0;
			m_BeatCount=-1;
			m_BarCount=0;
			m_NextBeatTime=0;
		}
		
		if (m_Time<0)
		{
			m_Time=m_Length;
		}
	}
}

void SequencerPlugin::ExecuteCommands()
{
	if (m_AudioCH->IsCommandWaiting())
	{
		switch (m_AudioCH->GetCommand())
		{
			case NEW_NOTE    : m_Track.AddNote(m_GUIArgs.Num,m_GUIArgs.Sequence,m_GUIArgs.t,m_GUIArgs.l,m_GUIArgs.n,m_GUIArgs.v); break;
			case CHG_NOTE    : m_Track.ChangeNote(m_GUIArgs.Num,m_GUIArgs.Sequence,m_GUIArgs.t,m_GUIArgs.l,m_GUIArgs.n,m_GUIArgs.v); break;
			case REM_NOTE    : m_Track.RemoveNote(m_GUIArgs.Num,m_GUIArgs.Sequence); break;
			case NEW_SEQ     : 
			{	
				m_Track.AddSequence(m_GUIArgs.Num); 
				m_Track.GetSequence(m_GUIArgs.Num)->SetColour(m_GUIArgs.Num2);
				m_Track.GetSequence(m_GUIArgs.Num)->SetName(m_GUIArgs.Name);
				m_Track.GetSequence(m_GUIArgs.Num)->SetLength(m_GUIArgs.l);
				m_Track.GetSequence(m_GUIArgs.Num)->SetYPos(m_GUIArgs.n);
				m_Track.GetSequence(m_GUIArgs.Num)->SetChannel(m_GUIArgs.Channel);
			} break;
			case COPY_SEQ    : m_Track.CopySequence(m_GUIArgs.Num,m_GUIArgs.Num2); break;
			case INST_SEQ    : m_Track.InstanceSequence(m_GUIArgs.Num); break;
			case CLONE_SEQ   : m_Track.CloneSequence(m_GUIArgs.Num,m_GUIArgs.Num2); break;
			case REM_SEQ     : m_Track.RemoveSequence(m_GUIArgs.Num); break;
			case CHG_SEQ     : 
			{
				m_Track.GetSequence(m_GUIArgs.Num)->SetColour(m_GUIArgs.Num2);
				m_Track.GetSequence(m_GUIArgs.Num)->SetName(m_GUIArgs.Name);
				m_Track.GetSequence(m_GUIArgs.Num)->SetLength(m_GUIArgs.l);
				m_Track.GetSequence(m_GUIArgs.Num)->SetYPos(m_GUIArgs.n);
				m_Track.GetSequence(m_GUIArgs.Num)->SetChannel(m_GUIArgs.Channel);
				m_Track.GetSequence(m_GUIArgs.Num)->SetStartTime(m_GUIArgs.t);
			} break;
			case GET_PATTERN : 
			{
				// start of transfer
				if (m_TransferPattern==-1) 
				{
					m_TransferPattern=m_Track.GetSequence(m_GUIArgs.Num)->GetPatternID();
					m_TransferNote=m_Track.GetPattern(m_TransferPattern)->m_NoteMap.begin();
					m_TransferCount=m_Track.GetPattern(m_TransferPattern)->GetNoteCount();
					
					cerr<<"going to transfer "<<m_TransferCount<<" notes"<<endl;
				}
				else
				{
					// last note gone
					if (m_TransferNote==m_Track.GetPattern(m_TransferPattern)->m_NoteMap.end())
					{
						m_TransferPattern=-1;
					}
					else
					{
						// copy this note over
						m_Transfer.Time     = m_TransferNote->second.Time;
						m_Transfer.Length   = m_TransferNote->second.Length;
						m_Transfer.MidiNote = m_TransferNote->second.MidiNote;
						m_Transfer.Vol      = m_TransferNote->second.Vol;
					
						m_TransferNote++;
					}
				}
			} break;
		 	default: break;
		}
	}
}

void SequencerPlugin::StreamOut(ostream &s)
{
	s<<m_Version<<" ";
	
	switch (m_Version)
	{
		case 1:
		{
			s<<m_Time<<" "<<m_Length<<" "<<m_BarLength<<" "<<m_BeatsPerBar<<" ";
			s<<m_Track;
		} break;
	}
}

void SequencerPlugin::StreamIn(istream &s)
{
	int version;
	s>>version;
	
	switch (version)
	{
		case 1:
		{
			s>>m_Time>>m_Length>>m_BarLength>>m_BeatsPerBar;
			s>>m_Track;
		} break;
	}
}
