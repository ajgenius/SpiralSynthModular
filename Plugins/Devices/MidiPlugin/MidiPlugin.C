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

#include <math.h>
#include <limits.h>
#include "MidiPlugin.h"
#include "NoteTable.h"
#include "Midi.h"
#include "EventBuffer.h"
using Spiral::Core::MidiDevice;
using Spiral::Core::MidiEvent;
#include "SpiralIcon.xpm"

using namespace std;
using namespace Spiral::Core;

int MidiPlugin::m_RefCount = 0;
bool MidiPlugin::m_OpenedDevice = false;

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
	return new MidiPlugin;
}

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DEVICE;
}

const char** SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}
}

///////////////////////////////////////////////////////

MidiPlugin::MidiPlugin() :
m_NoteLevel(0),
m_TriggerLevel(0),
m_PitchBendLevel(0),
m_ChannelPressureLevel(0),
m_AfterTouchLevel(0),
m_CurrentNote(0),
m_NextID(0)
{
	for (int n=0; n<1024; n++) m_Held[n].ID=-1;

	m_Version=2;
	
	if (m_RefCount==0 && !MidiDevice::Get())
	{
		MidiDevice::Init("SpiralModular",MidiDevice::READ);
		m_OpenedDevice=true;
	}
	
	m_RefCount++;

	m_PluginInfo.Name="Midi";
	m_PluginInfo.Width=80;
	m_PluginInfo.Height=140;
	m_PluginInfo.NumInputs=3;
	m_PluginInfo.NumOutputs=7;
	m_PluginInfo.PortTips.push_back("Note CV");
	m_PluginInfo.PortTips.push_back("Trigger CV");
	m_PluginInfo.PortTips.push_back("Events");
	m_PluginInfo.PortTips.push_back("Note CV");
	m_PluginInfo.PortTips.push_back("Trigger CV");
	m_PluginInfo.PortTips.push_back("PitchBend CV");
	m_PluginInfo.PortTips.push_back("ChannelPressure CV");
	m_PluginInfo.PortTips.push_back("Aftertouch CV");
	m_PluginInfo.PortTips.push_back("Clock CV");
	m_PluginInfo.PortTips.push_back("Events");
	
	for (int n=0; n<128; n++) m_ControlLevel[n]=0;
	
	SetUnsigned(DeviceVersion, 2);

	m_DeviceNum = SignedProperty(DefaultLinearFlags, 0, 0, 15, 1, 1);
	m_NoteCut = ToggleProperty(Property::WriteOnly, false);
	m_ContinuousNotes = ToggleProperty(Property::WriteOnly, false);

	RegisterSharedProperty(m_DeviceNum, "DeviceNum", "MIDI device number");
	RegisterSharedProperty(m_NoteCut, "NoteCut", "Clear pitch on note release");
	RegisterSharedProperty(m_ContinuousNotes, "ContinuousNotes", "Keep the trigger up between notes");
	m_Controls = VectorProperty(Property::WriteOnly);
	RegisterSharedPropertyWithKey(m_Controls, "CONTROLS", "Controls", "Controller output mappings");
	m_AudioCH->Register("CC",&m_GUIArgs.s);
	m_AudioCH->RegisterData("Name",ChannelHandler::INPUT,
		&m_GUIArgs.Name,sizeof(m_GUIArgs.Name));
	{
		std::vector<std::string> none, args;
		args.clear(); args.push_back("CC"); args.push_back("Name");
		m_AudioCH->RegisterCommand("ADDCONTROL", ADDCONTROL, args);
		m_AudioCH->RegisterCommand("DELCONTROL", DELCONTROL, none);
	}
}

MidiPlugin::~MidiPlugin()
{
        m_RefCount--;
	if (m_RefCount==0 && m_OpenedDevice)
	{
		MidiDevice::PackUpAndGoHome();
		m_OpenedDevice=false;
	}
}

PluginInfo &MidiPlugin::Initialise(const HostInfo *Host)
{	
	PluginInfo& Info= SpiralPlugin::Initialise(Host);		
	MidiDevice::SetDeviceName(Host->MIDIFILE);		
	PlaceEvents();
		
	return Info;
}

void MidiPlugin::Execute()
{

	// Done to clear IsEmpty field... 
	GetOutputBuf(0)->Zero();
	GetOutputBuf(1)->Zero();
	GetOutputBuf(2)->Zero();
	GetOutputBuf(3)->Zero();
	GetOutputBuf(4)->Zero();
	GetOutputBuf(5)->Zero();

	for (unsigned int c=0; c<m_Controls.size(); c++)
	{
		GetOutputBuf(c+5)->Zero();	
	}

	bool Triggered=false;

	SendEvents();
	Spiral::Core::EventBuffer *Events=GetEventOutput(m_PluginInfo.NumOutputs-1);
	if (Events) Events->Clear(m_HostInfo->BUFSIZE);

	// midi output	
	if (InputExists(0) && InputExists(1))
	{
		static bool TriggeredOut=false;
		if (GetInput(1,0)>0)
		{
			if (!TriggeredOut) // note on
			{				
				// get the midi note
				float Freq=GetInputPitch(0,0);
				int Note=0;
				for (int n=0; n<132; n++)
				{	
					if (feq(Freq,NoteTable[n],0.01f))
					{
						Note=n;
						break;
					}
				}
				
				MidiEvent NewEvent(MidiEvent::ON,Note,GetInput(1,0)*128.0f);
				MidiDevice::Get()->SendEvent(AsSigned(m_DeviceNum),NewEvent);
				TriggeredOut=true;
			}			
		}
		else
		{
			if (TriggeredOut) // note off
			{
				// get the midi note
				float Freq=GetInputPitch(0,0);
				int Note=0;
				for (int n=0; n<132; n++)
				{	
					if (feq(Freq,NoteTable[n],0.01f))
					{
						Note=n;
						break;
					}
				}

				MidiEvent NewEvent(MidiEvent::OFF,Note,0.0f);
				MidiDevice::Get()->SendEvent(AsSigned(m_DeviceNum),NewEvent);
				TriggeredOut=false;
			}			
		}		
	}
	
	MidiEvent Event=MidiDevice::Get()->GetEvent(AsSigned(m_DeviceNum));
	// get all the midi events since the last check
	while(Event.GetType()!=MidiEvent::NONE)
	{
		ReceiveEvent(Event);

		if (Event.GetType()==MidiEvent::ON)
		{	
			Triggered=true;
			m_CurrentNote=Event.GetNote();				
			m_NoteLevel=NoteTable[m_CurrentNote];			
			m_TriggerLevel=Event.GetVolume()/127.0f;
		}
			
		if (Event.GetType()==MidiEvent::OFF)
		{
			if (Event.GetNote()==m_CurrentNote)
			{
				m_TriggerLevel=0;
				if (AsBoolean(m_NoteCut)) m_NoteLevel=0;
			}
		}
		
		if (Event.GetType()==MidiEvent::PITCHBEND)
		{
			m_PitchBendLevel=Event.GetVolume()/127.0f*2.0f-1.0f;	
		}
		
		if (Event.GetType()==MidiEvent::CHANNELPRESSURE)
		{
			m_ChannelPressureLevel=Event.GetVolume()/127.0f;	
		}

		if (Event.GetType()==MidiEvent::AFTERTOUCH)
		{
			m_AfterTouchLevel=Event.GetVolume()/127.0f;
		}
		
		if (Event.GetType()==MidiEvent::PARAMETER)
		{			
			// just to make sure
			if (Event.GetNote()>=0 && Event.GetNote()<128)
			{
				m_ControlLevel[Event.GetNote()]=Event.GetVolume()/127.0f;							
			}
		}
		
		Event=MidiDevice::Get()->GetEvent(AsSigned(m_DeviceNum));
	}

	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		SetOutputPitch(0,n,m_NoteLevel);
		SetOutput(1,n,m_TriggerLevel);
		SetOutput(2,n,m_PitchBendLevel);
		SetOutput(3,n,m_ChannelPressureLevel);
		SetOutput(4,n,m_AfterTouchLevel);						
		SetOutput(5,n,MidiDevice::Get()->GetClock());						
	}
	
	for (unsigned int c=0; c<m_Controls.size(); c++)
	{
		GetOutputBuf(c+5)->Set(m_ControlLevel[AsSigned(Control(c))]);	
	}
	
	// make sure the trigger is registered if it's 
	// note is pressed before releasing the previous one.
	if (Triggered && !AsBoolean(m_ContinuousNotes)) SetOutput(1,0,0);
}

void  MidiPlugin::ExecuteCommands()
{
	// Process any commands from the GUI
	if (m_AudioCH->IsCommandWaiting())
	{
		switch (m_AudioCH->GetCommand())
		{
			case (ADDCONTROL) : AddControl(m_GUIArgs.s,m_GUIArgs.Name); break;
			case (DELCONTROL) : DeleteControl();
		};
	}
}

void MidiPlugin::AddControl(int s, const string &Name)
{
	VectorProperty *control = new VectorProperty(Property::WriteOnly);
	control->Add(new SignedProperty(DefaultLinearFlags, s, 0, 127, 1, 1));
	control->Add(new StringProperty(Property::WriteOnly, Name));
	m_Controls.Add(control);
	RemoveOutput();
	AddOutput();
	AddOutput();
	m_PluginInfo.NumOutputs++;
	m_PluginInfo.PortTips.pop_back();
	m_PluginInfo.PortTips.push_back(Name);
	m_PluginInfo.PortTips.push_back("Events");
	PlaceEvents();
	UpdatePluginInfoWithHost();
}

void MidiPlugin::DeleteControl()
{
	if (m_Controls.size()==0) return;
	
	m_Controls.RemoveLast();
	RemoveOutput();
	RemoveOutput();
	AddOutput();
	m_PluginInfo.NumOutputs--;
	m_PluginInfo.PortTips.pop_back();
	m_PluginInfo.PortTips.pop_back();
	m_PluginInfo.PortTips.push_back("Events");
	PlaceEvents();
	UpdatePluginInfoWithHost();
}

void MidiPlugin::PlaceEvents()
{
	m_PluginInfo.PortTypes.assign(m_PluginInfo.NumInputs+m_PluginInfo.NumOutputs, Sample::AUDIO);
	SetInPortType(m_PluginInfo, 2, Sample::EVENT);
	SetOutPortType(m_PluginInfo, m_PluginInfo.NumOutputs-1, Sample::EVENT);
}

void MidiPlugin::SendEvents()
{
	const Spiral::Core::EventBuffer *In=GetEventInput(2);
	for (unsigned i=0; In && i<In->Count(); i++)
	{
		const Spiral::Core::Event &Event=In->At(i);
		if (Event.Type==Spiral::Core::Event::NoteOn)
			MidiDevice::Get()->SendEvent(AsSigned(m_DeviceNum),MidiEvent(MidiEvent::ON,Event.NoteNum,Event.Volume*127.0f));
		else if (Event.Type==Spiral::Core::Event::NoteOff)
			MidiDevice::Get()->SendEvent(AsSigned(m_DeviceNum),MidiEvent(MidiEvent::OFF,Event.NoteNum,0.0f));
	}
}

void MidiPlugin::ReceiveEvent(const MidiEvent &In)
{
	Spiral::Core::EventBuffer *Events=GetEventOutput(m_PluginInfo.NumOutputs-1);
	Spiral::Core::Event Event;
	Event.Channel=AsSigned(m_DeviceNum);
	Event.NoteNum=In.GetNote();
	Event.Volume=In.GetVolume()/127.0f;
	if (!Events || Event.NoteNum<0 || Event.NoteNum>127 || Event.Volume<0 || Event.Volume>1) return;
	Event.Frequency=440.0*pow(2.0,(Event.NoteNum-69)/12.0);

	if (In.GetType()==MidiEvent::ON)
	{
		int slot=0;
		while (slot<1024 && m_Held[slot].ID>=0) slot++;
		if (slot==1024 || m_NextID==INT_MAX)
		{
			for (int n=0; n<1024; n++) m_Held[n].ID=-1;
			m_NextID=0;
			Event.Type=Spiral::Core::Event::Reset;
		}
		else
		{
			Event.Type=Spiral::Core::Event::NoteOn;
			Event.ID=++m_NextID;
			m_Held[slot].ID=Event.ID;
			m_Held[slot].Key=Event.NoteNum;
		}
	}
	else if (In.GetType()==MidiEvent::OFF)
	{
		int slot=-1;
		for (int n=0; n<1024; n++)
			if (m_Held[n].ID>=0 && m_Held[n].Key==Event.NoteNum && (slot<0 || m_Held[n].ID<m_Held[slot].ID))
				slot=n;
		if (slot<0) return;
		Event.Type=Spiral::Core::Event::NoteOff;
		Event.ID=m_Held[slot].ID;
		m_Held[slot].ID=-1;
	}
	else if (In.GetType()==MidiEvent::PARAMETER)
	{
		Event.Type=Spiral::Core::Event::Controller;
		Event.Parameter=Event.NoteNum;
		Event.NoteNum=0;
		Event.Value=Event.Volume;
		if (Event.Parameter==120 || Event.Parameter==123)
			for (int n=0; n<1024; n++) m_Held[n].ID=-1;
	}
	else if (In.GetType()==MidiEvent::PITCHBEND)
	{
		Event.Type=Spiral::Core::Event::PitchBend;
		Event.Value=Event.Volume*2-1;
	}
	else if (In.GetType()==MidiEvent::CHANNELPRESSURE)
	{
		Event.Type=Spiral::Core::Event::ChannelPressure;
		Event.Value=Event.Volume;
	}
	else if (In.GetType()==MidiEvent::AFTERTOUCH)
	{
		Event.Type=Spiral::Core::Event::NotePressure;
		Event.Value=Event.Volume;
	}
	else return;
	Events->Append(0,Event);
}

void MidiPlugin::Describe(Spiral::Core::Description &d) 
{
	d.Value(DeviceVersion).Separator(" ").Value(m_DeviceNum).Separator(" ").Value(m_NoteCut).Separator(" ");	
	d.Value(m_Controls.size()).Line();
	for (unsigned int n=0; n<m_Controls.size(); n++)
	{
		d.Value(Control(n)).Separator(" ").Value(ControlName(n).Value.size()).Separator(" ").Value(ControlName(n)).Line();
	}
	
}

void MidiPlugin::Apply(Spiral::Core::Description::Reader &r) 
{
	int version;
	r.Value(version);
		
	switch (version)
	{
		case 1:	r.Value(m_DeviceNum).Value(m_NoteCut); break;
		
		case 2:
		{
			r.Value(m_DeviceNum).Value(m_NoteCut); 
			
			int Num;
			r.Count(m_Controls, Num);
			
			for (int n=0; n<Num && r.More(); n++)
			{
				AddControl(0, "");
				int size;
				r.Value(Control(n));
				if (!r.Named()) r.Value(size);

				r.Value(ControlName(n));
				m_PluginInfo.PortTips[m_PluginInfo.NumInputs+6+n] = ControlName(n).Value;
			}			
		}
	}
}
