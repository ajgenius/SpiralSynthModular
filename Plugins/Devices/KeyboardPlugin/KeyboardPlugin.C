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

#include <limits.h>
#include "KeyboardPlugin.h"
#include "NoteTable.h"
#include "EventBuffer.h"
#include "SpiralIcon.xpm"

using namespace std;
using namespace Spiral::Core;

#include <config.h>

extern "C" {
const char *SpiralPlugin_GetHostVersion()
{
	return PACKAGE_VERSION;
}

const char *SpiralPlugin_GetHostABI()
{
	return SSM_HOST_ABI;
}

SpiralPlugin* SpiralPlugin_CreateInstance()
{
	return new KeyboardPlugin;
}

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DEVICE;
}


const char** SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}

int SpiralPlugin_GetID()
{
	return 40;
}

string SpiralPlugin_GetName()
{
	return "Keyboard";
}

string SpiralPlugin_GetGroupName()
{
	return "InputOutput";
}
}

///////////////////////////////////////////////////////

KeyboardPlugin::KeyboardPlugin() :
m_NoteLevel(0),
m_TriggerLevel(0),
m_HeldCount(0),
m_Retrigger(false),
m_PendingCount(0),
m_NextID(0)
{
	for (int n=0; n<MIDI_NOTES; n++) m_HeldID[n]=-1;

	m_Note = SignedProperty(DefaultLinearFlags, 0, 0, 127, 1, 1);
	RegisterSharedProperty(m_Note, "Note", "Note being played");
	{
		std::vector<std::string> none, args;
		args.clear(); args.push_back("Note");
		m_AudioCH->RegisterCommand("NOTE_OFF", NOTE_OFF, args);
		args.clear(); args.push_back("Note");
		m_AudioCH->RegisterCommand("NOTE_ON", NOTE_ON, args);
	}
	m_Version=0;
	SetUnsigned(DeviceVersion, 0);

	m_PluginInfo.Name="Keyboard";
	m_PluginInfo.Width=300;
	m_PluginInfo.Height=90;
	m_PluginInfo.NumInputs=0;
	m_PluginInfo.NumOutputs=3;
	m_PluginInfo.PortTips.push_back("Note CV");
	m_PluginInfo.PortTips.push_back("Trigger CV");
	m_PluginInfo.PortTips.push_back("Events");
		
}

KeyboardPlugin::~KeyboardPlugin()
{
}



PluginInfo &KeyboardPlugin::Initialise(const HostInfo *Host)
{
	PluginInfo &Info = SpiralPlugin::Initialise(Host);
	m_PluginInfo.PortTypes.assign(m_PluginInfo.NumInputs+m_PluginInfo.NumOutputs, Sample::AUDIO);
	SetOutPortType(m_PluginInfo, 2, Sample::EVENT);
	return Info;
}

void KeyboardPlugin::Execute()
{
	Spiral::Core::EventBuffer *Events=GetEventOutput(2);
	if (Events)
	{
		Events->Clear(m_HostInfo->BUFSIZE);
		for (int i=0; i<m_PendingCount; i++) Events->Append(0,m_Pending[i]);
	}
	m_PendingCount=0;

	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		SetOutputPitch(0,n,m_NoteLevel);
		SetOutput(1,n,m_TriggerLevel);				
	}

	if (m_Retrigger)
	{
		SetOutput(1,0,0.0f);
		m_Retrigger=false;
	}
}

void KeyboardPlugin::Play(int note)
{
	if (note<0 || note>=(int)(sizeof(NoteTable)/sizeof(NoteTable[0]))) return;
	m_Retrigger=m_TriggerLevel>0.0f;
	m_NoteLevel=NoteTable[note];
	m_TriggerLevel=1.0f;
}

void  KeyboardPlugin::ExecuteCommands()
{
	// Process any commands from the GUI
	if (m_AudioCH->IsCommandWaiting())
	{
		switch (m_AudioCH->GetCommand())
		{
			case NOTE_ON  :
			{
				NoteEvent(true);
				const int note=AsSigned(m_Note);
				int kept=0;
				for (int i=0; i<m_HeldCount; i++)
					if (m_Held[i]!=note) m_Held[kept++]=m_Held[i];
				m_HeldCount=kept;
				if (m_HeldCount==MAX_HELD)
				{
					for (int i=1; i<m_HeldCount; i++) m_Held[i-1]=m_Held[i];
					m_HeldCount--;
				}
				m_Held[m_HeldCount++]=note;
				Play(note);
				break;
			}
			case NOTE_OFF :
			{
				NoteEvent(false);
				const int note=AsSigned(m_Note);
				const bool newest=m_HeldCount>0 && m_Held[m_HeldCount-1]==note;
				int kept=0;
				for (int i=0; i<m_HeldCount; i++)
					if (m_Held[i]!=note) m_Held[kept++]=m_Held[i];
				m_HeldCount=kept;
				if (m_HeldCount==0) m_TriggerLevel=0.0f;
				else if (newest) Play(m_Held[m_HeldCount-1]);
				break;
			}
		};
	}
}

void KeyboardPlugin::NoteEvent(bool on)
{
	const int note=AsSigned(m_Note);
	if (note<0 || note>=MIDI_NOTES || m_PendingCount+2>MAX_PENDING) return;

	Spiral::Core::Event Event;
	Event.NoteNum=note;
	Event.Frequency=NoteTable[note];

	if (m_HeldID[note]>=0)
	{
		Event.Type=Spiral::Core::Event::NoteOff;
		Event.ID=m_HeldID[note];
		m_Pending[m_PendingCount++]=Event;
		m_HeldID[note]=-1;
	}

	if (!on) return;

	if (m_NextID==INT_MAX)
	{
		for (int n=0; n<MIDI_NOTES; n++) m_HeldID[n]=-1;
		m_NextID=0;
		Event.Type=Spiral::Core::Event::Reset;
		m_Pending[m_PendingCount++]=Event;
		return;
	}

	Event.Type=Spiral::Core::Event::NoteOn;
	Event.ID=++m_NextID;
	m_HeldID[note]=Event.ID;
	m_Pending[m_PendingCount++]=Event;
}

void KeyboardPlugin::Describe(Spiral::Core::Description &d) 
{
	d.Value(DeviceVersion).Line();
}

void KeyboardPlugin::Apply(Spiral::Core::Description::Reader &r) 
{
	int version;
	if (r.More()) r.Value(version);
}

