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
#include "SmoothPlugin.h"
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
	return new SmoothPlugin;
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
	return 0x0014;
}

string SpiralPlugin_GetName()
{
	return "Smooth";
}

string SpiralPlugin_GetGroupName()
{
	return "Control";
}
}

///////////////////////////////////////////////////////

SmoothPlugin::SmoothPlugin() :
m_Value(0)
{
	m_PluginInfo.Name="Smooth";
	m_PluginInfo.Width=120;
	m_PluginInfo.Height=80;
	m_PluginInfo.NumInputs=1;
	m_PluginInfo.NumOutputs=1;
	m_PluginInfo.PortTips.push_back("Input");	
	m_PluginInfo.PortTips.push_back("Output");
	
	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 1);

	m_Up = FloatProperty(DefaultLinearFlags, 0.5, 0, 1, 0.001, 0.01);
	m_Down = FloatProperty(DefaultLinearFlags, 0.5, 0, 1, 0.001, 0.01);

	RegisterSharedProperty(m_Up, "Up", "Up");
	RegisterSharedProperty(m_Down, "Down", "Down");


}

SmoothPlugin::~SmoothPlugin()
{
}

PluginInfo &SmoothPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);
}



void SmoothPlugin::Reset()
{
	ResetPorts();
	m_Value = 0;
}

void SmoothPlugin::Execute()
{
	float Value;
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		Value = GetInput(0,n);
		
		if (Value>m_Value) m_Value+=AsFloat(m_Up)*0.0001f;
		else m_Value-=AsFloat(m_Down)*0.0001f;
		
		SetOutput(0,n,m_Value);
	}
}

 
