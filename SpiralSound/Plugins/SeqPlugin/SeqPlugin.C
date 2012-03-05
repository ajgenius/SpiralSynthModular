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
#include "SeqPlugin.h"
#include "SeqPluginGUI.h"
#include <FL/Fl_Button.H>
#include "SpiralIcon.xpm"
#include "../../NoteTable.h"

using namespace std;

// for note on's
static const float TRIGGER_LEV=0.1;

extern "C" {
SpiralPlugin* SpiralPlugin_CreateInstance()
{
	return new SeqPlugin;
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

SeqPlugin::SeqPlugin() :
m_Time(0.0f),
m_Length(10.0f),
m_SpeedMod(1.0f),
m_CurrentNoteCV(0),
m_CurrentTriggerCV(0),
m_InNoteDown(false),
m_InNoteID(0),
m_CurrentPattern(0),
m_Triggered(false)
{
	m_Version=2;
	
	m_PluginInfo.Name="Sequencer";
	m_PluginInfo.Width=540;
	m_PluginInfo.Height=290;
	m_PluginInfo.NumInputs=4;
	m_PluginInfo.NumOutputs=2;		
	m_PluginInfo.PortTips.push_back("Play Trigger");	
	m_PluginInfo.PortTips.push_back("Speed CV");	
	m_PluginInfo.PortTips.push_back("Input Pitch CV");	
	m_PluginInfo.PortTips.push_back("Input Trigger CV");	
	m_PluginInfo.PortTips.push_back("Output Pitch");	
	m_PluginInfo.PortTips.push_back("Output Trigger");	
}

SeqPlugin::~SeqPlugin()
{
}

PluginInfo &SeqPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);
}

SpiralGUIType *SeqPlugin::CreateGUI()
{
	m_GUI = new SeqPluginGUI(m_PluginInfo.Width,
								  	    m_PluginInfo.Height,
										this,m_HostInfo);
	m_GUI->hide();
	return m_GUI;
}

void SeqPlugin::SetPattern(int s)          
{ 
	m_Eventmap[m_CurrentPattern]->hide();
	m_CurrentPattern=s;
	m_Eventmap[m_CurrentPattern]->show();
}


void SeqPlugin::Execute()
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
				m_InNoteID=m_Eventmap[m_CurrentPattern]->AddEventTime(m_Time, NoteNum, 0.5, Fl_SEvent::MELODY, false);
				
				m_InNoteTime=m_Time;
			}			
		}
		else
		{
			// Check trigger
			if (GetInput(3,n)<TRIGGER_LEV)
			{
				m_Eventmap[m_CurrentPattern]->SetEventLength(m_InNoteID, m_Time-m_InNoteTime);
				m_InNoteDown=false;
			}		
		}		
		
		
		
		// Get the notes from the map		
		vector<EventInfo> NoteVec=m_Eventmap[m_CurrentPattern]->GetEvents(m_Time);
		
		// play all the notes found
		for (vector<EventInfo>::iterator i=NoteVec.begin();
		     i!=NoteVec.end(); i++)
		{
			if (i->m_Type==EventInfo::START) 
			{					
				m_CurrentNoteCV=NoteTable[i->m_Group];
				m_CurrentTriggerCV=1;								
			}
				
			if (i->m_Type==EventInfo::END) 
			{	
				m_CurrentTriggerCV=0;
				if (m_NoteCut) m_CurrentNoteCV=0;
			}
		}

		SetOutputPitch(0,n,m_CurrentNoteCV);
		SetOutput(1,n,m_CurrentTriggerCV);
		m_Time+=Speed*m_SpeedMod;
		
		if (m_Time>m_Length)
		{
			m_Time=0;
		}
		
		if (m_Time<0)
		{
			m_Time=m_Length;
		}
	}
}

void SeqPlugin::StreamOut(ostream &s)
{
	s<<m_Version<<" ";
	
	switch (m_Version)
	{
		case 2:
		{
			s<<m_Time<<" ";
			s<<m_Length<<" ";
			s<<m_SpeedMod<<" ";
			s<<m_Loop<<" ";
			s<<m_NoteCut<<" ";
			s<<m_CurrentPattern<<" ";
		}
		// fallthrough
		
		case 1:
		{
			for(int n=0; n<NUM_PATTERNS; n++)
			{
				s<<*m_Eventmap[n]<<" ";
			}
		} break;
	}
}

void SeqPlugin::StreamIn(istream &s)
{
	int version;
	s>>version;
	
	switch (version)
	{
		case 2:
		{
			s>>m_Time;
			s>>m_Length;
			s>>m_SpeedMod;
			s>>m_Loop;
			s>>m_NoteCut;
			s>>m_CurrentPattern;
		}
		// fallthrough
		
		case 1:
		{
			for(int n=0; n<NUM_PATTERNS; n++)
			{
				s>>*m_Eventmap[n];
			}
		} break;
	}
}
