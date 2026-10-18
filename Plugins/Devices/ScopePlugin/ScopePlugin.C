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
#include "ScopePlugin.h"
#include "SpiralIcon.xpm"

using namespace std;
using namespace Spiral::Core;

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

SpiralPlugin* SpiralPlugin_CreateInstance() { return new ScopePlugin; }

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DEVICE;
}


const char** SpiralPlugin_GetIcon() { return SpiralIcon_xpm; }

int SpiralPlugin_GetID() { return 0x0001; }

string SpiralPlugin_GetName()
{
	return "Scope";
}

string SpiralPlugin_GetGroupName() { return "InputOutput"; }
}

///////////////////////////////////////////////////////

ScopePlugin::ScopePlugin()
{
	m_DataReady = ToggleProperty(Property::ReadOnly, false);
	m_DataSize = SignedProperty(Property::ReadOnly, 0, 0, 1048576, 1, 1);
	m_DataSizeChanged = ToggleProperty(Property::ReadOnly, false);
	RegisterSharedProperty(m_DataReady, "DataReady", "New data is ready");
	RegisterSharedProperty(m_DataSize, "DataSize", "Samples available");
	RegisterSharedProperty(m_DataSizeChanged, "DataSizeChanged", "Buffer size changed");
     m_PluginInfo.Name = "Scope";
     m_PluginInfo.Width = 260;
     m_PluginInfo.Height = 115;
     m_PluginInfo.NumInputs = 1;
     m_PluginInfo.NumOutputs = 1;
     m_PluginInfo.PortTips.push_back("Input");
     m_PluginInfo.PortTips.push_back("Output");
}

ScopePlugin::~ScopePlugin()
{
}

PluginInfo &ScopePlugin::Initialise(const HostInfo *Host)
{
	PluginInfo& Info = SpiralPlugin::Initialise(Host);
	AsSigned(m_DataSize) = m_HostInfo->BUFSIZE;
	m_Data = new float[AsSigned(m_DataSize)];
	m_AudioCH->RegisterData("AudioData",ChannelHandler::OUTPUT,m_Data,AsSigned(m_DataSize)*sizeof(float));
	return Info;
}



void ScopePlugin::Reset()
{
	ResetPorts();
	AsBoolean(m_DataReady) = false;
	delete m_Data;
	AsSigned(m_DataSize) = m_HostInfo->BUFSIZE;
	m_Data = new float[AsSigned(m_DataSize)];
	AsBoolean(m_DataSizeChanged) = true;
}

void ScopePlugin::Execute() {
     // Just copy the data through.
     AsBoolean(m_DataReady) = InputExists (0);
     if (GetOutputBuf (0)) GetOutputBuf (0)->Zero();
     if (AsBoolean(m_DataReady)) {
        GetOutputBuf (0)->Mix (*GetInput(0), 0);
        memcpy (m_Data, GetInput (0)->GetBuffer (), AsSigned(m_DataSize) * sizeof (float));
     }
}

void ScopePlugin::ExecuteCommands()
{
	if (m_AudioCH->IsCommandWaiting())
	{
		switch (m_AudioCH->GetCommand()) {
			case UPDATEDATASIZE :
			{
				m_AudioCH->ReplaceData("AudioData", m_Data, AsSigned(m_DataSize)*sizeof(float));
				AsBoolean(m_DataSizeChanged) = false;			
			}	
			break;
			
			default:
			{
			}
		}	
	}		
}
