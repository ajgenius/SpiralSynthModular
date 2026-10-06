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
#include "SpiralIcon.xpm"

using namespace std;

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
m_Volume(1.0f),
m_Hub(AudioTransportHub::Get()),
m_Registered(false)
{
	m_IsTerminal=true;
	m_NotifyOpenOut=false;
	m_ReportedMode=(int)m_Hub->GetMode();
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
	m_Hub->Attach(this,Host);
	m_Registered=true;
	return Info;
}



bool OutputPlugin::Kill()
{
	m_IsDead=true;
	if (!m_Registered) return true;

	m_Registered=false;
	m_Hub->Detach(this);
	return true;
}

void OutputPlugin::Reset()
{
	if (m_IsDead) return;
	ResetPorts();
	m_Hub->Reconfigure();
}

void OutputPlugin::Execute()
{
	if (m_IsDead)
		return;


	const Mode mode=GetMode();
	if (mode==OUTPUT || mode==DUPLEX)
		m_Hub->Transport()->SendStereo(GetInput(0),GetInput(1));

	if (mode==INPUT || mode==DUPLEX)
		m_Hub->Transport()->GetStereo(GetOutputBuf(0),GetOutputBuf(1));
}

void OutputPlugin::ExecuteCommands()
{
	if (m_IsDead || !m_AudioCH->IsCommandWaiting()) return;
	switch(m_AudioCH->GetCommand()) {
		case OPENREAD: m_Hub->OpenMode(AudioTransportHub::INPUT); break;
		case OPENWRITE: m_Hub->OpenMode(AudioTransportHub::OUTPUT); break;
		case OPENDUPLEX: m_Hub->OpenMode(AudioTransportHub::DUPLEX); break;
		case CLOSE: m_Hub->Close(); break;
		case SET_VOLUME: m_Hub->Transport()->SetVolume(m_Volume); break;
		case CLEAR_NOTIFY: m_NotifyOpenOut=false; break;
		default: break;
	}
}

void OutputPlugin::ServiceAudio()
{
	if (m_IsDead || !m_Hub->IsRepresentative(this)) return;

	const bool opening=GetMode()==NO_MODE;
	m_Hub->Service();
	if (opening) m_NotifyOpenOut=GetMode()==OUTPUT;
}
