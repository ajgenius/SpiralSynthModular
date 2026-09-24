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

#include "OutputPlugin.h"
#include <algorithm>
#include "SpiralIcon.xpm"

using namespace std;

static const HostInfo* host;
std::vector<OutputPlugin *> OutputPlugin::m_Members;
bool OutputPlugin::m_Configured=false;
OutputPlugin::Mode OutputPlugin::m_Mode=NO_MODE;

#include <config.h>

extern "C"
{
const char *SpiralPlugin_GetHostVersion()
{
	return PACKAGE_VERSION;
}

const char *SpiralPlugin_GetHostABI()
{
	return SSM_HOST_ABI;
}

SpiralPlugin* SpiralPlugin_CreateInstance() { return new OutputPlugin; }

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DSP;
}

const char** SpiralPlugin_GetIcon() { return SpiralIcon_xpm; }
int SpiralPlugin_GetID() { return 0x0000; }
string SpiralPlugin_GetName()
{
	return "Output";
}

string SpiralPlugin_GetGroupName() { return "InputOutput"; }
}

OutputPlugin::OutputPlugin() :
m_Volume(1.0f)
{
	m_Members.push_back(this);
	m_IsTerminal=true;
	m_NotifyOpenOut=false;
	m_ReportedMode=(int)m_Mode;
	m_AudioCH->Register("Mode",&m_ReportedMode,ChannelHandler::OUTPUT);
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
	Kill();
}

PluginInfo &OutputPlugin::Initialise(const HostInfo *Host)
{
	PluginInfo& Info= SpiralPlugin::Initialise(Host);
	host=Host;
	OUTPUTCLIENT::host = Host;
	string client = Host->AUDIOCLIENT;
	string dest   = Host->OUTPUTFILE;
	if (!m_Configured) {
		m_Configured = OUTPUTCLIENT::Get()->Configure(client, dest);
		if (!m_Configured) m_Mode=CLOSED;
	}
	OUTPUTCLIENT::Get()->AllocateBuffer();
	return Info;
}



bool OutputPlugin::Kill()
{
	m_IsDead=true;
	m_Members.erase(std::remove(m_Members.begin(),m_Members.end(),this),m_Members.end());
	if (m_Members.empty()) {
		if (cb_Blocking) cb_Blocking(m_Parent,false);
		OUTPUTCLIENT::PackUpAndGoHome();
		m_Mode=NO_MODE;
		m_Configured=false;
	}
	return true;
}

void OutputPlugin::ReportMode()
{
	for (size_t i=0;i<m_Members.size();++i)
		m_Members[i]->m_ReportedMode=(int)m_Mode;
}

void OutputPlugin::OpenMode(Mode mode)
{
	bool opened=false;
	m_Mode=CLOSED;
	if (m_Configured) {
		if (mode==INPUT) opened=OUTPUTCLIENT::Get()->OpenRead();
		if (mode==OUTPUT) opened=OUTPUTCLIENT::Get()->OpenWrite();
		if (mode==DUPLEX) opened=OUTPUTCLIENT::Get()->OpenReadWrite();
	}
	if (opened) m_Mode=mode;
	ReportMode();
	if (cb_Blocking) cb_Blocking(m_Parent,opened);
}

void OutputPlugin::Reset()
{
	if (m_IsDead) return;
	ResetPorts();
	const Mode previous=m_Mode;
	m_Configured=host && OUTPUTCLIENT::Get()->Configure(host->AUDIOCLIENT,host->OUTPUTFILE);
	OUTPUTCLIENT::Get()->AllocateBuffer();
	OpenMode(previous==NO_MODE ? OUTPUT : previous);
}

void OutputPlugin::Execute()
{
	if (m_IsDead)
		return;


	if (m_Mode==OUTPUT || m_Mode==DUPLEX)
		OUTPUTCLIENT::Get()->SendStereo(GetInput(0),GetInput(1));

	if (m_Mode==INPUT || m_Mode==DUPLEX)
		OUTPUTCLIENT::Get()->GetStereo(GetOutputBuf(0),GetOutputBuf(1));
}

void OutputPlugin::ExecuteCommands()
{
	if (m_IsDead || !m_AudioCH->IsCommandWaiting()) return;
	switch(m_AudioCH->GetCommand()) {
		case OPENREAD: OpenMode(INPUT); break;
		case OPENWRITE: OpenMode(OUTPUT); break;
		case OPENDUPLEX: OpenMode(DUPLEX); break;
		case CLOSE:
			OUTPUTCLIENT::Get()->Close();
			m_Mode=CLOSED;
			ReportMode();
			if (cb_Blocking) cb_Blocking(m_Parent,false);
			break;
		case SET_VOLUME: OUTPUTCLIENT::Get()->SetVolume(m_Volume); break;
		case CLEAR_NOTIFY: m_NotifyOpenOut=false; break;
		default: break;
	}
}

void OutputPlugin::ProcessAudio()
{
	if (m_IsDead || m_Members.empty() || m_Members.front()!=this) return;
	if (m_Mode==NO_MODE) {
		OpenMode(OUTPUT);
		m_NotifyOpenOut=m_Mode==OUTPUT;
	}
	bool ok=true;
	if (m_Mode==INPUT || m_Mode==DUPLEX) ok=OUTPUTCLIENT::Get()->Read();
	if (ok && (m_Mode==OUTPUT || m_Mode==DUPLEX)) ok=OUTPUTCLIENT::Get()->Play();
	if (!ok) {
		OUTPUTCLIENT::Get()->Close();
		m_Mode=CLOSED;
		ReportMode();
		if (cb_Blocking) cb_Blocking(m_Parent,false);
	}
}
