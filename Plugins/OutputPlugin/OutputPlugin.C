/*  SpiralSound
 *  Copyleft (C) 2001 David Griffiths <dave@pawfal.org>
 *
 *  OutputPlugin talks to OutputAudioClient, which picks a libspiralcore
 *  blocking AudioClient (PortAudio / ALSA / OSS) from HostInfo.AUDIOCLIENT.
 */

#include "OutputPlugin.h"
#include "OutputPluginGUI.h"
#include <FL/Fl_File_Chooser.H>
#include "SpiralIcon.xpm"

using namespace std;

static const HostInfo* host;
int OutputPlugin::m_RefCount=0;
int OutputPlugin::m_NoExecuted=0;
OutputPlugin::Mode OutputPlugin::m_Mode=NO_MODE;

extern "C"
{
SpiralPlugin* SpiralPlugin_CreateInstance() { return new OutputPlugin; }
char** SpiralPlugin_GetIcon() { return SsmXpmExport(SpiralIcon_xpm); }
int SpiralPlugin_GetID() { return 0x0000; }
string SpiralPlugin_GetGroupName() { return "InputOutput"; }
}

OutputPlugin::OutputPlugin() :
m_Volume(1.0f)
{
	m_RefCount++;
	m_IsTerminal=true;
	m_NotifyOpenOut=false;
	m_PluginInfo.Name="Output";
	m_PluginInfo.Width=100;
	m_PluginInfo.Height=100;
	m_PluginInfo.NumInputs=2;
	m_PluginInfo.NumOutputs=2;
	m_PluginInfo.PortTips.push_back("Left Out");
	m_PluginInfo.PortTips.push_back("Right Out");
	m_PluginInfo.PortTips.push_back("Left In");
	m_PluginInfo.PortTips.push_back("Right In");
	m_AudioCH->Register ("Volume", &m_Volume);
	m_AudioCH->Register ("OpenOut", &m_NotifyOpenOut, ChannelHandler::OUTPUT);
}

OutputPlugin::~OutputPlugin()
{
	m_RefCount--;
	if (m_RefCount==0)
	{
		cb_Blocking(m_Parent,false);
		OUTPUTCLIENT::PackUpAndGoHome();
		m_Mode=NO_MODE;
	}
}

PluginInfo &OutputPlugin::Initialise(const HostInfo *Host)
{
	PluginInfo& Info= SpiralPlugin::Initialise(Host);
	host=Host;
	OUTPUTCLIENT::host = Host;
	string client = Host->AUDIOCLIENT;
	string dest   = Host->OUTPUTFILE;
	OUTPUTCLIENT::Get()->Configure(client, dest);
	OUTPUTCLIENT::Get()->AllocateBuffer();
	return Info;
}

SpiralGUIType *OutputPlugin::CreateGUI()
{
	return new OutputPluginGUI(m_PluginInfo.Width, m_PluginInfo.Height, this, m_AudioCH, m_HostInfo);
}

bool OutputPlugin::Kill()
{
	m_IsDead=true;
	OUTPUTCLIENT::Get()->Kill();
	m_Mode=CLOSED;
	cb_Blocking(m_Parent,false);
	return true;
}

void OutputPlugin::Reset()
{
	if (m_IsDead) return;
	m_IsDead=true;
	OUTPUTCLIENT::Get()->Close();
	cb_Blocking(m_Parent,false);
	ResetPorts();
	if (host)
		OUTPUTCLIENT::Get()->Configure(host->AUDIOCLIENT, host->OUTPUTFILE);
	OUTPUTCLIENT::Get()->AllocateBuffer();

	switch (m_Mode)
	{
		case INPUT :
			OUTPUTCLIENT::Get()->OpenRead();
			cb_Blocking(m_Parent,true);
		break;
		case OUTPUT :
			OUTPUTCLIENT::Get()->OpenWrite();
			cb_Blocking(m_Parent,true);
		break;
		case DUPLEX :
			OUTPUTCLIENT::Get()->OpenReadWrite();
			cb_Blocking(m_Parent,true);
		break;
		default:{}
	}
	m_IsDead=false;
}

void OutputPlugin::Execute()
{
	if (m_IsDead)
		return;

	if (m_Mode==NO_MODE && m_RefCount==1)
	{
		if (OUTPUTCLIENT::Get()->OpenWrite())
		{
			cb_Blocking(m_Parent,true);
			m_Mode=OUTPUT;
			m_NotifyOpenOut=true;
		}
	}

	if (m_Mode==OUTPUT || m_Mode==DUPLEX)
		OUTPUTCLIENT::Get()->SendStereo(GetInput(0),GetInput(1));

	if (m_Mode==INPUT || m_Mode==DUPLEX)
		OUTPUTCLIENT::Get()->GetStereo(GetOutputBuf(0),GetOutputBuf(1));
}

void OutputPlugin::ExecuteCommands()
{
	if (m_IsDead)
		return;

	if (m_AudioCH->IsCommandWaiting())
	{
		switch(m_AudioCH->GetCommand())
		{
			case OPENREAD :
				if (OUTPUTCLIENT::Get()->OpenRead())
					m_Mode=INPUT;
			break;
			case OPENWRITE :
				if (OUTPUTCLIENT::Get()->OpenWrite())
				{
					m_Mode=OUTPUT;
					cb_Blocking(m_Parent,true);
				}
			break;
			case OPENDUPLEX :
				if (OUTPUTCLIENT::Get()->OpenReadWrite())
				{
					m_Mode=DUPLEX;
					cb_Blocking(m_Parent,true);
				}
			break;
			case CLOSE :
				m_Mode=CLOSED;
				cb_Blocking(m_Parent,false);
				OUTPUTCLIENT::Get()->Close();
			break;
			case SET_VOLUME :
				OUTPUTCLIENT::Get()->SetVolume(m_Volume);
				break;
			case CLEAR_NOTIFY:
				m_NotifyOpenOut=false;
				break;
			default: break;
		}
	}
}

void OutputPlugin::ProcessAudio()
{
	if (m_IsDead)
		return;

	m_NoExecuted--;
	if (m_NoExecuted<=0)
	{
		if (m_Mode==INPUT || m_Mode==DUPLEX) OUTPUTCLIENT::Get()->Read();
		if (m_Mode==OUTPUT || m_Mode==DUPLEX) OUTPUTCLIENT::Get()->Play();
		m_NoExecuted=m_RefCount;
	}
}
