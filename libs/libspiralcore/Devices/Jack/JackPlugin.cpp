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
#include "JackIcon.xpm"
#include "AudioTiming.h"
#include "DeviceClassRegistry.h"
#include <iostream>

using namespace std;
using namespace Spiral::Core;

int JackPlugin::JackInstanceCount = 0;
static std::map<long, JackPlugin *> liveGroups;

void JackPlugin::ClaimGroup(long desired)
{
	for (std::map<long, JackPlugin *>::iterator i = liveGroups.begin(); i != liveGroups.end(); ++i)
		if (i->second == this)
		{
			liveGroups.erase(i);
			break;
		}
	if (desired < 1 || liveGroups.count(desired))
	{
		desired = 1;
		while (liveGroups.count(desired)) ++desired;
	}
	liveGroups[desired] = this;
	AsSigned(m_Group) = desired;
}


const unsigned char *JackManifest(size_t *size);

static SpiralPlugin *CreateInstance()
{
	return new JackPlugin;
}

const DeviceClass &JackPlugin::Class()
{
	static DeviceClass device;

	if (device.ID < 0)
	{
		size_t size = 0;
		const unsigned char *manifest = JackManifest(&size);
		std::string error;

		if (!DeviceClassRegistry::ReadManifest(manifest, size, device, error))
			std::cerr << "JackPlugin: " << error << std::endl;

		device.Icon = SpiralIcon_xpm;
		device.Create = CreateInstance;
	}

	return device;
}

static const BuiltinDevice Registration(JackPlugin::Class);

///////////////////////////////////////////////////////

JackPlugin::JackPlugin() :
m_NextAttach(0),
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
	m_AudioCH->Register("RouteTicket", &m_GUIArgs.RouteTicket);
	m_AudioCH->RegisterByteSnapshot("RouteChoices");
	SetString(DeviceName, Class().Name);
	SetUnsigned(DeviceVersion, 2);

	int PortFlags = DefaultLinearFlags | Property::ForceUpdate;

	m_NumInputs = SignedProperty(PortFlags, 4, MIN_PORTS, MAX_PORTS, 1, 1);
	m_NumOutputs = SignedProperty(PortFlags, 4, MIN_PORTS, MAX_PORTS, 1, 1);

	m_Group = SignedProperty(Property::WriteOnly | Property::Internal, 1, 1, 0x7fffffff, 1, 1);
	m_LegacyClient = StringProperty(Property::WriteOnly | Property::Internal, "");
	RegisterSharedProperty(m_Group, "JACK GROUP", "Persistent JACK port group");
	RegisterSharedProperty(m_LegacyClient, "JACK CLIENT", "Dedicated name for legacy connections");
	ClaimGroup(m_JackInstanceID + 1);

	RegisterSharedProperty(m_NumInputs, "Inputs", "Number of Input Channels");
	RegisterSharedProperty(m_NumOutputs, "Outputs", "Number of Output Channels");
	m_AudioCH->RegisterData("Port",ChannelHandler::INPUT,&m_GUIArgs.Port,sizeof(m_GUIArgs.Port));
	m_NumInputPortNames = SignedProperty(Property::ReadOnly, 0, 0, MAX_PORTS, 1, 1);
	m_NumOutputPortNames = SignedProperty(Property::ReadOnly, 0, 0, MAX_PORTS, 1, 1);
	m_Attached = ToggleProperty(Property::WriteOnly | Property::Internal, false);
	RegisterSharedProperty(m_Attached, "ATTACHED", "Requested JACK connection");
	m_Connected = ToggleProperty(Property::ReadOnly, false);
	RegisterSharedProperty(m_NumInputPortNames, "NumInputPortNames", "Jack input ports found");
	RegisterSharedProperty(m_NumOutputPortNames, "NumOutputPortNames", "Jack output ports found");
	m_Requested = ToggleProperty(Property::ReadOnly, false);
	RegisterSharedProperty(m_Requested, "REQUESTED", "JACK connection requested");
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
		args.clear();
		args.push_back("PortIndex");
		args.push_back("RouteTicket");
		m_AudioCH->RegisterCommand("LIST_ROUTES", LIST_ROUTES, args, "ii");
		args.push_back("Port");
		m_AudioCH->RegisterCommand("ADD_ROUTE", ADD_ROUTE, args, "iis");
		m_AudioCH->RegisterCommand("REMOVE_ROUTE", REMOVE_ROUTE, args, "iis");
	}
}

JackPlugin::~JackPlugin()
{
	liveGroups.erase(AsSigned(m_Group));
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
	AsBoolean(m_Attached) = true;
	OpenClient(false);
}

void JackPlugin::OpenClient(bool deferOpen)
{
	m_NextAttach = AudioMonotonicTime() + 1;
	std::string name = m_LegacyClient.Value;
	if (name.empty())
	{
		char prefix[40];
		snprintf(prefix, sizeof(prefix), "Jack%ld_", long(AsSigned(m_Group)));
		m_JackClient->SetPortGroup(AudioTransportHub::Get(), prefix, deferOpen);
		name = m_HostInfo && m_HostInfo->AUDIOCLIENT == "jack" ? m_HostInfo->OUTPUTFILE : "default";
		if (name.empty() || name == "default") name = "SSM-Output";
	}
	else
		m_JackClient->SetPortGroup(this, "", deferOpen, true);
	Spiral::Core::AudioClientOptions options;
	options.InChannels = m_InputCount;
	options.OutChannels = m_OutputCount;
	const bool opened = m_JackClient->HasPortGroup() && m_JackClient->IsAttached()
		? m_JackClient->ResizePortGroup(options.InChannels, options.OutChannels)
		: m_JackClient->Attach(name, options);
	if (opened)
	{
		BuildRings();
		if (!m_Stream.Configure(m_JackClient, m_InputCount, m_OutputCount,
			m_HostInfo->BUFSIZE, m_HostInfo->SAMPLERATE)) { CloseClient(); return; }

		m_JackClient->SetCallback(Spiral::Core::AudioStream::Callback, &m_Stream);
		AudioTransportHub::Get()->RegisterStream(&m_Stream);
		if (!m_JackClient->Start()) CloseClient();
	}

}

void JackPlugin::Detach()
{
	AsBoolean(m_Attached) = false;
	CloseClient();
}

void JackPlugin::CloseClient()
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
			case ATTACH: AsBoolean(m_Attached) = true; m_NextAttach = 0; break;
			case DETACH: Detach(); RefreshConnectionEcho(); break;
			case CONNECT_INPUT: ConnectInput(m_GUIArgs.PortIndex, m_GUIArgs.Port); RefreshConnectionEcho(); break;
			case CONNECT_OUTPUT: ConnectOutput(m_GUIArgs.PortIndex, m_GUIArgs.Port); RefreshConnectionEcho(); break;
			case DISCONNECT_INPUT: m_JackClient->DisconnectInput(m_GUIArgs.PortIndex); RefreshConnectionEcho(); break;
			case DISCONNECT_OUTPUT: m_JackClient->DisconnectOutput(m_GUIArgs.PortIndex); RefreshConnectionEcho(); break;

			case LIST_ROUTES:
				m_JackClient->RequestRoutes(m_GUIArgs.PortIndex, m_GUIArgs.RouteTicket);
				break;
			case ADD_ROUTE:
			case REMOVE_ROUTE:
				m_JackClient->RequestRoutes(m_GUIArgs.PortIndex, m_GUIArgs.RouteTicket,
					m_GUIArgs.Port, m_AudioCH->GetCommand() == ADD_ROUTE);
				break;

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
	std::string routes;
	if (m_JackClient->TakeRoutes(routes))
		m_AudioCH->PublishBytes("RouteChoices", routes);
	AsBoolean(m_Requested) = AsBoolean(m_Attached);
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
	if (reconnect && m_JackClient->HasPortGroup())
	{
		Attach();
		return;
	}
	if (reconnect) CloseClient();
	BuildRings();
	if (reconnect) Attach();
}

void JackPlugin::ServiceAudio()
{
	if (m_IsDead) return;
	if (!m_JackClient->IsAttached())
	{
		if (AsBoolean(m_Attached) && (m_JackClient->SessionReady() || AudioMonotonicTime() >= m_NextAttach))
			OpenClient(true);
		return;
	}

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
     const bool shared = reconnect && m_JackClient->HasPortGroup();
     // Register the new native ports before changing the graph. JACK can
     // refuse growth while the existing ports and their cables are still usable.
     if (shared && !m_JackClient->ResizePortGroup(nOutputs, nInputs))
     {
         AsSigned(m_NumInputs) = m_OutputCount;
         AsSigned(m_NumOutputs) = m_InputCount;
         m_JackClient->Start();
         return;
     }
     if (!shared) CloseClient();
     const int oldInputs = m_PluginInfo.NumInputs;
     const int oldOutputs = m_PluginInfo.NumOutputs;
     CreatePorts (nInputs, nOutputs, false);
     UpdatePluginInfoWithHost();
     for (int n = nInputs; n < oldInputs; ++n) RemoveInput();
     for (int n = nOutputs; n < oldOutputs; ++n) RemoveOutput();
     for (int n = oldInputs; n < nInputs; ++n) AddInput();
     for (int n = oldOutputs; n < nOutputs; ++n) AddOutput();
     if (shared)
     {
         ResetPorts();
         Attach();
     }
     else
     {
         Reset();
         if (reconnect) Attach();
     }

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
	d.Value(DeviceVersion).Separator(" ").Value(m_NumInputs).Separator(" ").Value(m_NumOutputs).Separator(" ")
		.Value(m_Group).Separator(" ").Value(m_LegacyClient).Separator(" ").Value(m_Attached).Separator(" ");
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
	
	const long previousGroup = AsSigned(m_Group);
	const std::string previousClient = m_LegacyClient.Value;
	if (r.Find("JACK GROUP") >= 0)
	{
		r.Value(m_Group).Value(m_LegacyClient);
		ClaimGroup(AsSigned(m_Group));
	}
	else
	{
		char name[32];
		snprintf(name, sizeof(name), "SSM%d", m_JackInstanceID);
		m_LegacyClient.Value = name;
	}

	// A pasted copy must not claim its original's external JACK identity.
	if (!m_LegacyClient.Value.empty())
	{
		for (std::map<long, JackPlugin *>::const_iterator i = liveGroups.begin(); i != liveGroups.end(); ++i)
			if (i->second != this && i->second->m_LegacyClient.Value == m_LegacyClient.Value)
			{
				m_LegacyClient.Value.clear();
				break;
			}
	}

	if (r.Find("ATTACHED") >= 0) r.Value(m_Attached);
	const bool wanted = AsBoolean(m_Attached);
	const bool changeEndpoint = m_JackClient->IsAttached()
		&& (previousClient != m_LegacyClient.Value
			|| (m_LegacyClient.Value.empty() && previousGroup != AsSigned(m_Group)));
	if (changeEndpoint || !wanted) CloseClient();

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

	AsBoolean(m_Attached) = wanted;
	m_NextAttach = 0;
}
