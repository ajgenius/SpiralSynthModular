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
#include <string>
#include <iostream>
#include <stdio.h>

#include "SplitterPlugin.h"
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
	return new SplitterPlugin;
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

SplitterPlugin::SplitterPlugin()
{
	SetUnsigned(DeviceVersion, 2);

	m_ChannelCount = SignedProperty(Property::ForceUpdate|DefaultLinearFlags, 4, 2, 256, 1, 1);
	RegisterSharedProperty(m_ChannelCount, "Channels", "Channels");
	{
		std::vector<std::string> none, args;
		m_AudioCH->RegisterCommand("SETCHANNELCOUNT", SETCHANNELCOUNT, none);
	}
	m_PluginInfo.Name="Splitter";
	m_PluginInfo.Width=80;
	m_PluginInfo.Height=40;

	m_Version = 2;
	AsSigned(m_ChannelCount) = 4;
	CreatePorts ();

}

SplitterPlugin::~SplitterPlugin()
{
}

void SplitterPlugin::CreatePorts (int n, bool AddPorts) 
{
	int c;
	char t[256];
	
	m_PluginInfo.NumInputs = 1;
	m_PluginInfo.PortTips.push_back ("Input");

	m_PluginInfo.NumOutputs = n;
	for (c=1; c<=n; c++) {
		sprintf (t, "Out %d", c);
		m_PluginInfo.PortTips.push_back (t);
	}

	if (AddPorts) {
		for (int c=0; c<m_PluginInfo.NumInputs; c++) AddInput();
		for (int c=0; c<m_PluginInfo.NumOutputs; c++) AddOutput();
	}
}

void SplitterPlugin::SetChannelCount (int count) 
{
	AsSigned(m_ChannelCount) = count;

	UpdatePluginInfoWithHost();

	RemoveAllInputs ();
	RemoveAllOutputs ();
	m_PluginInfo.NumInputs = 0;
	m_PluginInfo.NumOutputs = 0;
	m_PluginInfo.PortTips.clear ();

	CreatePorts (count, true);
	UpdatePluginInfoWithHost ();
}

PluginInfo &SplitterPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise( Host);
}

void SplitterPlugin::Execute()
{
	// Just copy the data through.
	for (int i=0; i<GetChannelCount(); i++)
	{
		GetOutputBuf(i)->Zero();
	}
	
	if (InputExists(0))  
	{
		for (int i=0; i<GetChannelCount(); i++)
		{
			GetOutputBuf(i)->Mix(*GetInput(0),0);
		}	
	}
}

void SplitterPlugin::ExecuteCommands () 
{
	if (m_AudioCH->IsCommandWaiting ()) 
	{
		switch (m_AudioCH->GetCommand()) {
			case SETCHANNELCOUNT : 
			{
				UpdatePluginInfoWithHost();

				RemoveAllInputs ();
				RemoveAllOutputs ();
				m_PluginInfo.NumInputs = 0;
				m_PluginInfo.NumOutputs = 0;
				m_PluginInfo.PortTips.clear ();

				CreatePorts (AsSigned(m_ChannelCount), true);
				UpdatePluginInfoWithHost ();
			}
			break;
		}
	}
}

void SplitterPlugin::Describe(Spiral::Core::Description &d) 
{
	d.Value(m_Version).Separator(" ").Value(m_ChannelCount).Separator(" ");
}

void SplitterPlugin::Apply(Spiral::Core::Description::Reader &r) 
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
			r.Value(m_ChannelCount);
			m_ChannelCount.Clamp();
			SetChannelCount (AsSigned(m_ChannelCount));
		}
		break;
		
		case 1:
		{
			//use original fixed defaults
			SetChannelCount (4);
		}
		break;
	}	
}
