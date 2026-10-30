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
#include "OperatorPlugin.h"
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
	return new OperatorPlugin;
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
	return 44;
}

string SpiralPlugin_GetName()
{
	return "Operator";
}

string SpiralPlugin_GetGroupName()
{
	return "Maths/Logic";
}
}

///////////////////////////////////////////////////////

OperatorPlugin::OperatorPlugin()
{
	m_Version=2;

	m_PluginInfo.Name="Operator";
	m_PluginInfo.Width=120;
	m_PluginInfo.Height=80;
	m_PluginInfo.NumInputs=2;
	m_PluginInfo.NumOutputs=1;
	m_PluginInfo.PortTips.push_back("Input 1");	
	m_PluginInfo.PortTips.push_back("Input 2");	
	m_PluginInfo.PortTips.push_back("Output");
		
	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 2);

	m_Constant = FloatProperty(DefaultLinearFlags,0,0,1,0.01,0.1);
	RegisterSharedProperty(m_Constant, "Constant", "Constant");
	m_Operator = SignedProperty(DefaultLinearFlags,ADD,NONE,MUL,1,1);
	RegisterSharedProperty(m_Operator, "Operator", "Operator");


}

OperatorPlugin::~OperatorPlugin()
{
}

PluginInfo &OperatorPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);
}



void OperatorPlugin::Execute()
{	
	float Freq=0, OldFreq=0;
	switch (AsSigned(m_Operator)) 
	{
		case ADD : 
			if (InputExists(1))
			{
				for (int n=0; n<m_HostInfo->BUFSIZE; n++)
				{
					SetOutput(0,n,GetInput(0,n)+GetInput(1,n));
				} 
			}
			else
			{
				for (int n=0; n<m_HostInfo->BUFSIZE; n++)
				{
					SetOutput(0,n,GetInput(0,n)+AsFloat(m_Constant));
				} 
			}
			break;
		case SUB : 
			if (InputExists(1))
			{
				for (int n=0; n<m_HostInfo->BUFSIZE; n++)
				{
					SetOutput(0,n,GetInput(0,n)-GetInput(1,n));
				} 
			}
			else
			{
				for (int n=0; n<m_HostInfo->BUFSIZE; n++)
				{
					SetOutput(0,n,GetInput(0,n)-AsFloat(m_Constant));
				} 
			}
			break;
		case DIV : 
			if (InputExists(1))
			{
				for (int n=0; n<m_HostInfo->BUFSIZE; n++)
				{
					if (GetInput(1,n)==0) SetOutput(0,n,0);
					else SetOutput(0,n,GetInput(0,n)/GetInput(1,n));
				} 
			}
			else
			{
				for (int n=0; n<m_HostInfo->BUFSIZE; n++)
				{
					if (AsFloat(m_Constant)==0) SetOutput(0,n,0);
					else SetOutput(0,n,GetInput(0,n)/AsFloat(m_Constant));
				} 
			}
			break;
		case MUL :
			if (InputExists(1))
			{
				for (int n=0; n<m_HostInfo->BUFSIZE; n++)
				{
					SetOutput(0,n,GetInput(0,n)*GetInput(1,n));
				}
			}
			else
			{
				for (int n=0; n<m_HostInfo->BUFSIZE; n++)
				{
					SetOutput(0,n,GetInput(0,n)*AsFloat(m_Constant));
				}
			}
			break;
                default:
                        break;
	}
}

void OperatorPlugin::ExecuteCommands()
{
}

void OperatorPlugin::Migrate(Spiral::Core::Description &d)
{
	if (d.Version() != 1)
		return;

	if (d.Values().size() == 2)
		d.Value(AsSigned(m_Operator));
	d.SetValue(0, 2);
}
