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

#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <algorithm>

#include "config.h"
#include "JackPlugin.h"
#include "AudioTransportHub.h"
#include "SpiralIcon.xpm"

using namespace std;
using namespace Spiral::Core;

int JackPlugin::JackInstanceCount = 0;


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
	return new JackPlugin;
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
	return 31;
}

string SpiralPlugin_GetName()
{
	return "Jack";
}

string SpiralPlugin_GetGroupName()
{
	return "InputOutput";
}
}

///////////////////////////////////////////////////////

JackPlugin::JackPlugin() :
m_UpdateNames(false),
m_RingFrames(0),
m_NativeRate(0),
m_CaptureFrame(0),
m_InputCount(4),
m_OutputCount(4)
{
        m_JackClient=new JackClient;

	//clunky way to ensure unique JackID - JackInstanceCount is never dec 
	//so new JackInstances per session always get a higher number even on 
	//reload and new Patch
	
	m_JackInstanceID = JackInstanceCount;
	JackInstanceCount++;

	for (int n = 0; n < m_InputCount; ++n) m_JackClient->AddInputPort();

	for (int n = 0; n < m_OutputCount; ++n) m_JackClient->AddOutputPort();
        
	// we are an output
	m_IsTerminal = true;
	
	m_Version = 2;
	
	m_PluginInfo.Name="Jack";
	m_PluginInfo.Width=225;
	m_PluginInfo.Height=230;
	m_PluginInfo.NumInputs=0;
	m_PluginInfo.NumOutputs=0;
	
     	m_PluginInfo.PortTips.clear();

	m_PluginInfo.NumInputs = m_OutputCount;
	AsSigned(m_NumInputs) = m_PluginInfo.NumInputs;

	for (int n=0; n<m_InputCount; n++)
     	{
		char Temp[256];
		sprintf(Temp,"SSM Input %d",n);
		m_PluginInfo.PortTips.push_back(Temp);
     	}
	
	m_PluginInfo.NumOutputs = m_InputCount;
	AsSigned(m_NumOutputs) = m_PluginInfo.NumOutputs;

	for (int n=0; n<m_OutputCount; n++)
	{
		char Temp[256];
		sprintf(Temp,"SSM Output %d",n);
		m_PluginInfo.PortTips.push_back(Temp);
	}
     	
	m_AudioCH->Register("PortIndex",&m_GUIArgs.PortIndex);
	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 2);

	int PortFlags = DefaultLinearFlags | Property::ForceUpdate;

	m_NumInputs = SignedProperty(PortFlags, 4, MIN_PORTS, MAX_PORTS, 1, 1);
	m_NumOutputs = SignedProperty(PortFlags, 4, MIN_PORTS, MAX_PORTS, 1, 1);

	RegisterSharedProperty(m_NumInputs, "Inputs", "Number of Input Channels");
	RegisterSharedProperty(m_NumOutputs, "Outputs", "Number of Output Channels");
	m_AudioCH->RegisterData("Port",ChannelHandler::INPUT,&m_GUIArgs.Port,sizeof(m_GUIArgs.Port));
	m_NumInputPortNames = SignedProperty(Property::ReadOnly, 0, 0, MAX_PORTS, 1, 1);
	m_NumOutputPortNames = SignedProperty(Property::ReadOnly, 0, 0, MAX_PORTS, 1, 1);
	m_Connected = ToggleProperty(Property::ReadOnly, false);
	RegisterSharedProperty(m_NumInputPortNames, "NumInputPortNames", "Jack input ports found");
	RegisterSharedProperty(m_NumOutputPortNames, "NumOutputPortNames", "Jack output ports found");
	RegisterSharedProperty(m_Connected, "Connected", "Attached to Jack");
	m_AudioCH->RegisterData("InputPortNames",ChannelHandler::OUTPUT,&m_InputPortNames,sizeof(m_InputPortNames));
	m_AudioCH->RegisterData("OutputPortNames",ChannelHandler::OUTPUT,&m_OutputPortNames,sizeof(m_OutputPortNames));
	m_AudioCH->Register("UpdateNames",&m_UpdateNames,ChannelHandler::OUTPUT);	
	memset(m_InputConnections, 0, sizeof(m_InputConnections));
	memset(m_OutputConnections, 0, sizeof(m_OutputConnections));
	m_AudioCH->RegisterData("InputConnections",ChannelHandler::OUTPUT,&m_InputConnections,sizeof(m_InputConnections));
	m_AudioCH->RegisterData("OutputConnections",ChannelHandler::OUTPUT,&m_OutputConnections,sizeof(m_OutputConnections));
	{
		std::vector<std::string> none, args;
		m_AudioCH->RegisterCommand("ATTACH", ATTACH, none);
		m_AudioCH->RegisterCommand("CHECK_PORT_CHANGES", CHECK_PORT_CHANGES, none);
		args.clear(); args.push_back("PortIndex"); args.push_back("Port");
		m_AudioCH->RegisterCommand("CONNECT_INPUT", CONNECT_INPUT, args);
		args.clear(); args.push_back("PortIndex"); args.push_back("Port");
		m_AudioCH->RegisterCommand("CONNECT_OUTPUT", CONNECT_OUTPUT, args);
		m_AudioCH->RegisterCommand("DETACH", DETACH, none);
		args.clear(); args.push_back("PortIndex");
		m_AudioCH->RegisterCommand("DISCONNECT_INPUT", DISCONNECT_INPUT, args);
		args.clear(); args.push_back("PortIndex");
		m_AudioCH->RegisterCommand("DISCONNECT_OUTPUT", DISCONNECT_OUTPUT, args);
		m_AudioCH->RegisterCommand("UPDATE_NAMES", UPDATE_NAMES, none);
	}
}

JackPlugin::~JackPlugin()
{
	if (m_JackClient)
	{
		Detach();
		delete m_JackClient; 
		m_JackClient=NULL;
	}

	DropRings();
}

PluginInfo &JackPlugin::Initialise(const HostInfo *Host)
{	
	PluginInfo& Info= SpiralPlugin::Initialise(Host);

	AudioTransportHub::Get()->SetHost(Host);
	Reset();
	return Info;
}



void JackPlugin::Attach()
{
	char name[32];
	sprintf(name, "SSM%d", m_JackInstanceID);
	Spiral::Core::AudioClientOptions options;
	options.InChannels = m_InputCount;
	options.OutChannels = m_OutputCount;
	if (m_JackClient->Attach(name, options))
	{
		BuildRings();
		if (!m_Stream.Configure(m_JackClient, m_InputCount, m_OutputCount,
			m_HostInfo->BUFSIZE, m_HostInfo->SAMPLERATE)) { Detach(); return; }

		m_JackClient->SetCallback(Spiral::Core::AudioStream::Callback, &m_Stream);
		AudioTransportHub::Get()->RegisterStream(&m_Stream);
		if (!m_JackClient->Start()) Detach();
	}

}

void JackPlugin::Detach()
{
	AudioTransportHub::Get()->UnregisterStream(&m_Stream);
	m_JackClient->Detach();
}

void JackPlugin::BuildRings()
{
	if (!m_HostInfo || m_HostInfo->BUFSIZE <= 0) return;

	m_RingFrames = m_JackClient->GetBufferSize();
	m_NativeRate = m_JackClient->GetSampleRate();
	m_EnginePeriod.assign(m_HostInfo->BUFSIZE * std::max(m_InputCount, m_OutputCount), 0);
	m_CaptureFrame = 0;
}

void JackPlugin::DropRings()
{
	m_EnginePeriod.clear();
	m_RingFrames = 0;
}

void JackPlugin::Execute()
{
	if (m_IsDead || m_EnginePeriod.empty() || !m_JackClient->IsAttached()) return;

	const unsigned frames = m_HostInfo->BUFSIZE;

	for (unsigned frame = 0; frame < frames; ++frame)
		for (int channel = 0; channel < m_OutputCount; ++channel)
			m_EnginePeriod[frame * m_OutputCount + channel] = !m_HostInfo->PAUSED && InputExists(channel)
				? (*GetInput(channel))[frame] : 0;

	m_Stream.Playback(&m_EnginePeriod[0], frames, AudioTransportHub::Get()->PlaybackStamp());
}

void JackPlugin::ExecuteCommands()
{
	if (m_IsDead) return;
	const int inputs = std::max(MIN_PORTS, std::min(MAX_PORTS, (int)AsSigned(m_NumInputs)));
	const int outputs = std::max(MIN_PORTS, std::min(MAX_PORTS, (int)AsSigned(m_NumOutputs)));
	if (inputs != m_OutputCount || outputs != m_InputCount)
	{
		SetNumberPorts (inputs, outputs);
		RefreshConnectionEcho();
	}
	
	if (m_AudioCH->IsCommandWaiting())
	{
		switch (m_AudioCH->GetCommand()) {
			case ATTACH: Attach(); RefreshConnectionEcho(); break;
			case DETACH: Detach(); RefreshConnectionEcho(); break;
			case CONNECT_INPUT: ConnectInput(m_GUIArgs.PortIndex, m_GUIArgs.Port); RefreshConnectionEcho(); break;
			case CONNECT_OUTPUT: ConnectOutput(m_GUIArgs.PortIndex, m_GUIArgs.Port); RefreshConnectionEcho(); break;
			case DISCONNECT_INPUT: m_JackClient->DisconnectInput(m_GUIArgs.PortIndex); RefreshConnectionEcho(); break;
			case DISCONNECT_OUTPUT: m_JackClient->DisconnectOutput(m_GUIArgs.PortIndex); RefreshConnectionEcho(); break;

			case UPDATE_NAMES :
			{
				int c=0;
	
			    std::vector<string> InputNames,OutputNames;
				GetPortNames(InputNames,OutputNames);
				for (vector<string>::iterator i=InputNames.begin();
					 i!=InputNames.end() && c<MAX_PORTS; ++i)
				{
					snprintf(m_InputPortNames[c], sizeof(m_InputPortNames[c]), "%s", i->c_str());
					c++;
				}
		
				c=0;
		
				for (std::vector<string>::iterator i=OutputNames.begin();
					 i!=OutputNames.end() && c<MAX_PORTS; ++i)
				{
					snprintf(m_OutputPortNames[c], sizeof(m_OutputPortNames[c]), "%s", i->c_str());
					c++;
				}
		
				AsSigned(m_NumInputPortNames)=std::min((int)InputNames.size(), MAX_PORTS);
				AsSigned(m_NumOutputPortNames)=std::min((int)OutputNames.size(), MAX_PORTS);
			}

			break;
			
			case CHECK_PORT_CHANGES :
				RefreshConnectionEcho();
				break;

			default : break;
		}
	}
	AsBoolean(m_Connected)=m_JackClient->IsAttached();
}

void JackPlugin::RefreshConnectionEcho()
{
	for (int n=0; n<MAX_PORTS; n++)
	{
		const std::string in = n < m_InputCount ? m_JackClient->GetInputConnection(n) : std::string();
		const std::string out = n < m_OutputCount ? m_JackClient->GetOutputConnection(n) : std::string();
		snprintf(m_InputConnections[n], sizeof(m_InputConnections[n]), "%s", in.c_str());
		snprintf(m_OutputConnections[n], sizeof(m_OutputConnections[n]), "%s", out.c_str());
	}
}

bool JackPlugin::Kill()
{
	m_IsDead=true;
	if (m_JackClient) Detach();

	return true;
}

void JackPlugin::Reset()
{
	ResetPorts();
	if (!m_HostInfo) return;

	const bool reconnect = m_JackClient->IsAttached();
	if (reconnect) Detach();
	BuildRings();
	if (reconnect) Attach();
}

void JackPlugin::ServiceAudio()
{
	if (m_IsDead || !m_JackClient->IsAttached()) return;

	if (m_JackClient->GetBufferSize() != m_RingFrames ||
		m_JackClient->GetSampleRate() != m_NativeRate || m_Stream.Failed()) Reset();
}

void JackPlugin::ProcessAudio()
{
	if (m_IsDead || m_EnginePeriod.empty()) return;

	const unsigned frames = m_HostInfo->BUFSIZE;
	const bool silent = m_HostInfo->PAUSED || !m_JackClient->IsAttached();
	const Spiral::Core::AudioStamp &stamp = AudioTransportHub::Get()->CaptureStamp();
	if (!silent) m_Stream.Capture(&m_EnginePeriod[0], frames, stamp);

	m_CaptureFrame = stamp.Frame;
	for (unsigned frame = 0; frame < frames; ++frame)
		for (int channel = 0; channel < m_InputCount; ++channel)
			if (OutputExists(channel))
				GetOutputBuf(channel)->Set(frame, silent ? 0 : m_EnginePeriod[frame * m_InputCount + channel]);

}

void  JackPlugin::SetNumberPorts (int nInputs, int nOutputs) {
     nInputs = std::max(MIN_PORTS, std::min(MAX_PORTS, nInputs));
     nOutputs = std::max(MIN_PORTS, std::min(MAX_PORTS, nOutputs));
     const bool reconnect = m_JackClient->IsAttached();
     Detach();
     const int oldInputs = m_PluginInfo.NumInputs;
     const int oldOutputs = m_PluginInfo.NumOutputs;
     CreatePorts (nInputs, nOutputs, false);
     UpdatePluginInfoWithHost();
     for (int n = nInputs; n < oldInputs; ++n) RemoveInput();
     for (int n = nOutputs; n < oldOutputs; ++n) RemoveOutput();
     for (int n = oldInputs; n < nInputs; ++n) AddInput();
     for (int n = oldOutputs; n < nOutputs; ++n) AddOutput();
     Reset();
     if (reconnect) Attach();

}

void  JackPlugin::CreatePorts (int nInputs, int nOutputs, bool AddPorts) {
        nInputs = std::max(MIN_PORTS, std::min(MAX_PORTS, nInputs));
        nOutputs = std::max(MIN_PORTS, std::min(MAX_PORTS, nOutputs));
    	m_PluginInfo.PortTips.clear();

    	m_PluginInfo.NumInputs = nInputs;
	m_OutputCount = nInputs;

     	for (int n=0; n<nInputs; n++)
     	{
		char Temp[256];
		sprintf(Temp,"SSM Input %d",n);
		m_PluginInfo.PortTips.push_back(Temp);
     	}
	
    	m_PluginInfo.NumOutputs = nOutputs;
	m_InputCount = nOutputs;

	for (int n=0; n<nOutputs; n++)
	{
		char Temp[256];
		sprintf(Temp,"SSM Output %d",n);
		m_PluginInfo.PortTips.push_back(Temp);
	}

     if (AddPorts) {
        for (int n=0; n<nInputs; n++) AddInput();
        for (int n=0; n<nOutputs; n++) AddOutput();
     }

}

void JackPlugin::Describe(Spiral::Core::Description &d) 
{
	d.Value(DeviceVersion).Separator(" ").Value(m_NumInputs).Separator(" ").Value(m_NumOutputs).Separator(" ");
}

void JackPlugin::Apply(Spiral::Core::Description::Reader &r) 
{
	int Version;

	if (r.More())
	{
		r.Value(Version);
	}
	else
	{
		//No Version, so use Version 1
		Version = 1;
	}
	
	switch (Version)
	{
		case 2:
		{
			r.Value(m_NumInputs).Value(m_NumOutputs);
			AsSigned(m_NumOutputs) = min(max(AsSigned(m_NumOutputs), MIN_PORTS), MAX_PORTS);
			AsSigned(m_NumInputs) = min(max(AsSigned(m_NumInputs), MIN_PORTS), MAX_PORTS);
			
			SetNumberPorts (AsSigned(m_NumInputs), AsSigned(m_NumOutputs));				
		}
		break;
		
		case 1:
		{
			//use original fixed defaults
			AsSigned(m_NumInputs) = 16;
			AsSigned(m_NumOutputs) = 16;

			SetNumberPorts (AsSigned(m_NumInputs), AsSigned(m_NumOutputs));				
		}
		break;
	}	
}
