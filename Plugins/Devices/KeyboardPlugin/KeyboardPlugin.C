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

#include "KeyboardPlugin.h"
#include "NoteTable.h"
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
m_Retrigger(false)
{
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
	m_PluginInfo.NumOutputs=2;
	m_PluginInfo.PortTips.push_back("Note CV");
	m_PluginInfo.PortTips.push_back("Trigger CV");
		
}

KeyboardPlugin::~KeyboardPlugin()
{
}



void KeyboardPlugin::Execute()
{
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		SetOutputPitch(0,n,m_NoteLevel);
		SetOutput(1,n,m_TriggerLevel);				
	}

	// A new key, or letting go of the newest while others are held, drops the trigger
	// for one sample, as the old editor's note off then note on did, so envelopes start again.
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

void KeyboardPlugin::Describe(Spiral::Core::Description &d) 
{
	d.Value(DeviceVersion).Line();
}

void KeyboardPlugin::Apply(Spiral::Core::Description::Reader &r) 
{
	int version;
	if (r.More()) r.Value(version);
}

