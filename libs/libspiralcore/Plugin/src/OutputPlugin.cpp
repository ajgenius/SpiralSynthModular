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
#include "OutputIcon.xpm"
#include "DeviceClassRegistry.h"
#include "Finite.h"
#include <algorithm>
#include <iostream>

using namespace std;

static SpiralPlugin *CreateInstance() { return new OutputPlugin; }

const unsigned char *OutputManifest(size_t *size);

const Spiral::Core::DeviceClass &OutputPlugin::Class()
{
	static Spiral::Core::DeviceClass device;

	if (device.ID < 0)
	{
		size_t size = 0;
		const unsigned char *manifest = OutputManifest(&size);
		std::string error;

		if (!Spiral::Core::DeviceClassRegistry::ReadManifest(manifest, size, device, error))
			std::cerr << "Output: " << error << std::endl;

		device.Icon = SpiralIcon_xpm;
		device.Create = CreateInstance;
	}

	return device;
}

static const Spiral::Core::BuiltinDevice Registration(OutputPlugin::Class);

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
	m_PluginInfo.PortTips.push_back("Left In");
	m_PluginInfo.PortTips.push_back("Right In");
	m_PluginInfo.PortTips.push_back("Left Out");
	m_PluginInfo.PortTips.push_back("Right Out");
	m_AudioCH->Register ("Volume", &m_Volume);
	m_AudioCH->Register ("OpenOut", &m_NotifyOpenOut, ChannelHandler::OUTPUT);
	const std::vector<std::string> none;
	m_AudioCH->RegisterCommand("OPENREAD", OPENREAD, none);
	m_AudioCH->RegisterCommand("OPENWRITE", OPENWRITE, none);
	m_AudioCH->RegisterCommand("OPENDUPLEX", OPENDUPLEX, none);
	m_AudioCH->RegisterCommand("CLOSE", CLOSE, none);
	m_AudioCH->RegisterCommand("SET_VOLUME", SET_VOLUME, none);
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

void OutputPlugin::Describe(Spiral::Core::Description &d)
{
	d.Value(1).Separator(" ");
	d.Value(m_Hub->Transport()->GetVolume()).Key("VOLUME", "FloatProperty");
}

void OutputPlugin::Apply(Spiral::Core::Description::Reader &r)
{
	const int at = r.Find("VOLUME");
	float volume = 0;
	if (at < 0 || !r.At((size_t)at, volume) || !Spiral::Core::IsFinite(volume))
		return;
	m_Volume = std::max(0.0f, std::min(1.0f, volume));
	m_Hub->Transport()->SetVolume(m_Volume);
}

void OutputPlugin::ServiceAudio()
{
	if (m_IsDead || !m_Hub->IsRepresentative(this)) return;

	const bool opening=GetMode()==NO_MODE;
	m_Hub->Service();
	if (opening) m_NotifyOpenOut=GetMode()==OUTPUT;
}
