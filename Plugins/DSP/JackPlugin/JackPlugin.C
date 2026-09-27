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

#include "config.h"
#include "JackPlugin.h"
#include "SpiralIcon.xpm"

using namespace std;

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
	return SPIRAL_PLUGIN_TYPE_DSP;
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
m_Connected(false),
m_InputCount(4),
m_OutputCount(4)
{
	pthread_mutex_init(&m_TransferLock, NULL);
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
	m_GUIArgs.NumInputs = m_PluginInfo.NumInputs;

	for (int n=0; n<m_InputCount; n++)
     	{
		char Temp[256];
		sprintf(Temp,"SSM Input %d",n);
		m_PluginInfo.PortTips.push_back(Temp);
     	}
	
	m_PluginInfo.NumOutputs = m_InputCount;
	m_GUIArgs.NumOutputs = m_PluginInfo.NumOutputs;

	for (int n=0; n<m_OutputCount; n++)
	{
		char Temp[256];
		sprintf(Temp,"SSM Output %d",n);
		m_PluginInfo.PortTips.push_back(Temp);
	}
     	
	m_AudioCH->Register("PortIndex",&m_GUIArgs.PortIndex);
	m_AudioCH->Register("NumInputs",&m_GUIArgs.NumInputs);
	m_AudioCH->Register("NumOutputs",&m_GUIArgs.NumOutputs);
	m_AudioCH->RegisterData("Port",ChannelHandler::INPUT,&m_GUIArgs.Port,sizeof(m_GUIArgs.Port));
	m_AudioCH->Register("NumInputPortNames",&m_NumInputPortNames,ChannelHandler::OUTPUT);
	m_AudioCH->Register("NumOutputPortNames",&m_NumOutputPortNames,ChannelHandler::OUTPUT);
	m_AudioCH->RegisterData("InputPortNames",ChannelHandler::OUTPUT,&m_InputPortNames,sizeof(m_InputPortNames));
	m_AudioCH->RegisterData("OutputPortNames",ChannelHandler::OUTPUT,&m_OutputPortNames,sizeof(m_OutputPortNames));
	m_AudioCH->Register("UpdateNames",&m_UpdateNames,ChannelHandler::OUTPUT);	
	m_AudioCH->Register("Connected",&m_Connected,ChannelHandler::OUTPUT);	
}

JackPlugin::~JackPlugin()
{
	if (m_JackClient)
	{
		m_JackClient->Detach();
		delete m_JackClient; 
		m_JackClient=NULL;
	}

	pthread_mutex_destroy(&m_TransferLock);
}

PluginInfo &JackPlugin::Initialise(const HostInfo *Host)
{	
	PluginInfo& Info= SpiralPlugin::Initialise(Host);

	m_JackClient->SetCallback(ProcessCallback, this);
	Reset();
	return Info;
}



void JackPlugin::Attach()
{
	char name[32];
	sprintf(name, "SSM%d", m_JackInstanceID);
	spiralcore::AudioClientOptions options;
	options.InChannels = m_InputCount;
	options.OutChannels = m_OutputCount;
	if (m_JackClient->Attach(name, options))
		m_JackClient->Start();

}

void JackPlugin::Detach()
{
	m_JackClient->Detach();
}

void JackPlugin::ProcessCallback(void *context, unsigned int frames)
{
	JackPlugin *plugin = static_cast<JackPlugin *>(context);
	// Capture cannot depend on entering the host: a blocking output can
	// hold its gate for an entire device period. Retain the latest block,
	// then let either the callback or host loop deliver it to the graph.
	if (frames && !pthread_mutex_trylock(&plugin->m_TransferLock))
	{
		if (plugin->m_Capture.size() == frames * plugin->m_InputCount)
			plugin->m_JackClient->Read(&plugin->m_Capture[0], frames);

		pthread_mutex_unlock(&plugin->m_TransferLock);
	}

	plugin->RunAudioCycle(frames);

	if (frames && !pthread_mutex_trylock(&plugin->m_TransferLock))
	{
		if (plugin->m_Playback.size() == frames * plugin->m_OutputCount)
			plugin->m_JackClient->Write(&plugin->m_Playback[0], frames);

		pthread_mutex_unlock(&plugin->m_TransferLock);
	}

}

void JackPlugin::Execute()
{
	// A blocking output may drive the graph while JACK callbacks only fill
	// the exchange buffers. Deliver that capture when this node executes.
	ProcessAudio();
}

void JackPlugin::ExecuteCommands()
{
	if (m_IsDead) return;
	
	if (m_AudioCH->IsCommandWaiting())
	{
		switch (m_AudioCH->GetCommand()) {
			case ATTACH: Attach(); break;
			case DETACH: Detach(); break;
			case CONNECT_INPUT: ConnectInput(m_GUIArgs.PortIndex, m_GUIArgs.Port); break;
			case CONNECT_OUTPUT: ConnectOutput(m_GUIArgs.PortIndex, m_GUIArgs.Port); break;
			case DISCONNECT_INPUT: m_JackClient->DisconnectInput(m_GUIArgs.PortIndex); break;
			case DISCONNECT_OUTPUT: m_JackClient->DisconnectOutput(m_GUIArgs.PortIndex); break;
			case SET_PORT_COUNT :
				SetNumberPorts (m_GUIArgs.NumInputs, m_GUIArgs.NumOutputs);				
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
		
				m_NumInputPortNames=std::min((int)InputNames.size(), MAX_PORTS);
				m_NumOutputPortNames=std::min((int)OutputNames.size(), MAX_PORTS);
			}

			break;
			
			case CHECK_PORT_CHANGES :
				break;

			default : break;
		}
	}
	m_Connected=m_JackClient->IsAttached();
}

bool JackPlugin::Kill()
{
	m_IsDead=true;
	if (m_JackClient) m_JackClient->Detach();

	// The host may already have removed neighboring devices during a patch
	// replacement. Stop callbacks here; leave port and canvas disposal to the
	// normal destruction path rather than issuing live topology updates.
	return true;
}

void JackPlugin::Reset()
{
	ResetPorts();
	if (!m_HostInfo) return;

	pthread_mutex_lock(&m_TransferLock);
	m_Capture.assign(m_InputCount * m_HostInfo->BUFSIZE, 0);
	m_Playback.assign(m_OutputCount * m_HostInfo->BUFSIZE, 0);
	pthread_mutex_unlock(&m_TransferLock);
}

void JackPlugin::ServiceAudio()
{
	if (m_IsDead) return;

	if (!m_JackClient->IsAttached()) return;

	if (ChangeBufferAndSampleRate)
		ChangeBufferAndSampleRate(m_JackClient->GetBufferSize(), m_JackClient->GetSampleRate(), m_Parent);

}

void JackPlugin::ProcessAudio()
{
	if (m_IsDead || m_Capture.empty() || m_Playback.empty()) return;

	const unsigned frames = m_HostInfo->BUFSIZE;
	if (m_Capture.size() != frames * m_InputCount || m_Playback.size() != frames * m_OutputCount) return;

	pthread_mutex_lock(&m_TransferLock);
	const bool silent = m_HostInfo->PAUSED || !m_JackClient->IsAttached();
	for (unsigned frame = 0; frame < frames; ++frame)
	{
		for (int channel = 0; channel < m_InputCount; ++channel)
			if (OutputExists(channel))
				GetOutputBuf(channel)->Set(frame, silent ? 0 : m_Capture[frame * m_InputCount + channel]);

		for (int channel = 0; channel < m_OutputCount; ++channel)
			m_Playback[frame * m_OutputCount + channel] = !silent && InputExists(channel)
				? (*GetInput(channel))[frame] : 0;
	}

	pthread_mutex_unlock(&m_TransferLock);
}

void  JackPlugin::SetNumberPorts (int nInputs, int nOutputs) {
     nInputs = std::max(MIN_PORTS, std::min(MAX_PORTS, nInputs));
     nOutputs = std::max(MIN_PORTS, std::min(MAX_PORTS, nOutputs));
     const bool reconnect = m_JackClient->IsAttached();
     Detach();
     UpdatePluginInfoWithHost();
     RemoveAllInputs ();
     RemoveAllOutputs ();
     m_PluginInfo.NumInputs = 0;
     m_PluginInfo.NumOutputs = 0;
     m_PluginInfo.PortTips.clear ();
     CreatePorts (nInputs, nOutputs, true);
     Reset();
     UpdatePluginInfoWithHost ();
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

void JackPlugin::StreamOut (ostream &s) 
{
	s << m_Version << " " << m_GUIArgs.NumInputs << " " << m_GUIArgs.NumOutputs << " ";
}

void JackPlugin::StreamIn (istream &s) 
{
	char Test;
	int Version, NumInputs, NumOutputs;

	s.seekg (2, ios::cur );  //skip to next line
	Test = s.peek();         //peek first char
	s.seekg (-2, ios::cur ); //jump back to prior line
	
	if ( (Test >= '0') && (Test <= '9') )
	{
		s >> Version;
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
			s >> NumInputs >> NumOutputs;
			m_GUIArgs.NumOutputs = min(max(NumOutputs, MIN_PORTS), MAX_PORTS);
			m_GUIArgs.NumInputs = min(max(NumInputs, MIN_PORTS), MAX_PORTS);
			
			SetNumberPorts (m_GUIArgs.NumInputs, m_GUIArgs.NumOutputs);				
		}
		break;
		
		case 1:
		{
			//use original fixed defaults
			m_GUIArgs.NumInputs = 16;
			m_GUIArgs.NumOutputs = 16;

			SetNumberPorts (m_GUIArgs.NumInputs, m_GUIArgs.NumOutputs);				
		}
		break;
	}	
}
