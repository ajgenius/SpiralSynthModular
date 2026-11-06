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
#include "CounterPlugin.h"
#include "SpiralIcon.xpm"
#include "NoteTable.h"

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
	return new CounterPlugin;
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

CounterPlugin::CounterPlugin() :
m_Triggered(false),
m_CurrentLevel(1.0f)
{
	m_PluginInfo.Name="Counter";
	m_PluginInfo.Width=80;
	m_PluginInfo.Height=50;
	m_PluginInfo.NumInputs=1;
	m_PluginInfo.NumOutputs=1;
	m_PluginInfo.PortTips.push_back("Input");	
	m_PluginInfo.PortTips.push_back("Output");

	SetUnsigned(DeviceVersion, 1);

	m_Count = SignedProperty(DefaultLinearFlags, 4, 0, 10000, 1, 1);
	RegisterSharedProperty(m_Count, "Count", "Count");
	m_Current = SignedProperty(Property::Internal, 0, 0, 10000, 1, 1);
	RegisterSharedProperty(m_Current, "Current", "Current count");
}

CounterPlugin::~CounterPlugin()
{
}

PluginInfo &CounterPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);
}

void CounterPlugin::Reset()
{
	ResetPorts();
	AsSigned(m_Current) = 0;
	m_Triggered = false;
	m_CurrentLevel = 1.0f;
}

void CounterPlugin::Execute()
{	
	bool Triggered;
	
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		if (GetInput(0,n)>0)
		{
			if(!m_Triggered)
			{
				m_Triggered=true;
				AsSigned(m_Current)++;
			}
		}
		else
		{
			if (m_Triggered)
			{
				m_Triggered=false;
				AsSigned(m_Current)++;
			}
		}

		if (AsSigned(m_Current)>=AsSigned(m_Count))
		{
			m_CurrentLevel=-m_CurrentLevel;
			AsSigned(m_Current)=0;
		}
		
		SetOutput(0,n,m_CurrentLevel);
	}
}

void CounterPlugin::ExecuteCommands()
{
}
	
