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
#include "FlipflopPlugin.h"
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
	return new FlipflopPlugin;
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
	return 46;
}

string SpiralPlugin_GetName()
{
	return "Flipflop";
}

string SpiralPlugin_GetGroupName()
{
	return "Maths/Logic";
}
}

///////////////////////////////////////////////////////

FlipflopPlugin::FlipflopPlugin() :
m_Current(0),
m_Triggered(false),
m_CurrentLevel(1.0f),
m_TriggerSamples(0)
{
	m_Version = 2;
	m_PluginInfo.Name="Flipflop";
	m_PluginInfo.Width=80;
	m_PluginInfo.Height=100;
	m_PluginInfo.NumInputs=1;
	m_PluginInfo.NumOutputs=1;
	m_PluginInfo.PortTips.push_back("Input");	
	m_PluginInfo.PortTips.push_back("Output");

	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 2);

	m_TriggerTime = FloatProperty(DefaultLinearFlags, 0.01, 0, 1, 0.0001f, 0.001f);
	RegisterSharedProperty(m_TriggerTime, "TriggerTime", "TriggerTime");
	m_Monostable = ToggleProperty(Property::WriteOnly,false);
	RegisterSharedProperty(m_Monostable, "Monostable", "Monostable");


}

FlipflopPlugin::~FlipflopPlugin()
{
}

PluginInfo &FlipflopPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);
}



void FlipflopPlugin::Reset()
{
	ResetPorts();
	m_Current = 0;
	m_Triggered = false;
	m_CurrentLevel = 1.0f;
	m_TriggerSamples = 0;
}

void FlipflopPlugin::Execute()
{	
	bool Triggered;
	
	if (AsBoolean(m_Monostable))
	{
		for (int n=0; n<m_HostInfo->BUFSIZE; n++)
		{
			if (GetInput(0,n)>0)
			{
				if(!m_Triggered)
				{
					m_Triggered=true;
					m_CurrentLevel=1.0f;
					m_TriggerSamples=(int)(AsFloat(m_TriggerTime)*m_HostInfo->SAMPLERATE)+1;
				}
			}
			else
			{
				m_Triggered=false;
			}
			
			if (m_TriggerSamples<=0)
			{
				m_CurrentLevel=-1.0f;
			}
			
			if (m_TriggerSamples>0) m_TriggerSamples--;
			
			SetOutput(0,n,m_CurrentLevel);
		}	
	}
	else
	{
		for (int n=0; n<m_HostInfo->BUFSIZE; n++)
		{
			if (GetInput(0,n)>0)
			{
				if(!m_Triggered)
				{
					m_Triggered=true;
					m_CurrentLevel=-m_CurrentLevel;
				}
			}
			else
			{
				if (m_Triggered)
				{
					m_Triggered=false;
				}
			}
			
			SetOutput(0,n,m_CurrentLevel);
		}
	}
}

void FlipflopPlugin::ExecuteCommands()
{
}
	
void FlipflopPlugin::Migrate(Spiral::Core::Description &d)
{
	if (d.Version() != 1)
		return;

	if (d.Values().size() == 1)
		d.Value(AsFloat(m_TriggerTime)).Value(AsBoolean(m_Monostable));
	d.SetValue(0, 2);
}
