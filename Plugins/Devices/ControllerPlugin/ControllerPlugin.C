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

#include <stdio.h>
#include "ControllerPlugin.h"
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
	return new ControllerPlugin;
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
	return 0x0003;
}

string SpiralPlugin_GetName()
{
	return "CV";
}

string SpiralPlugin_GetGroupName()
{
	return "Control";
}
}

///////////////////////////////////////////////////////

ControllerPlugin::ControllerPlugin()
{
	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 5);

	m_NumChannels = UnsignedProperty(DefaultLinearFlags, 4, 1, MAX_CHANNELS, 1, 1);
	RegisterSharedProperty(m_NumChannels, "Num", "Number of channels");
	m_ChannelVal = VectorProperty(Property::WriteOnly);
	for (int n=0; n<MAX_CHANNELS; n++) m_ChannelVal.Add(new FloatProperty(DefaultLinearFlags, 0.0f, 0.0f, 1.0f, 0.001f, 0.01f));
	m_MinVal = VectorProperty(Property::WriteOnly);
	for (int n=0; n<MAX_CHANNELS; n++) m_MinVal.Add(new FloatProperty(DefaultLinearFlags, 0.0f, 0.0f, 1.0f, 0.001f, 0.01f));
	m_MaxVal = VectorProperty(Property::WriteOnly);
	for (int n=0; n<MAX_CHANNELS; n++) m_MaxVal.Add(new FloatProperty(DefaultLinearFlags, 0.0f, 0.0f, 1.0f, 0.001f, 0.01f));
	m_Names = VectorProperty(Property::WriteOnly);
	for (int n=0; n<MAX_CHANNELS; n++) m_Names.Add(new StringProperty(Property::WriteOnly, ""));
	RegisterSharedProperty(m_ChannelVal, "ChannelVal", "Channel values");
	RegisterSharedProperty(m_MinVal, "MinVal", "Lower bounds");
	RegisterSharedProperty(m_MaxVal, "MaxVal", "Upper bounds");
	RegisterSharedProperty(m_Names, "Names", "Channel names");

	m_Version=5;

	m_PluginInfo.Name="CV";
	m_PluginInfo.Width=240;
	m_PluginInfo.Height=224;
	m_PluginInfo.NumInputs=0;
	m_PluginInfo.NumOutputs=4;
	m_PluginInfo.PortTips.push_back("CV 1");
	m_PluginInfo.PortTips.push_back("CV 2");
	m_PluginInfo.PortTips.push_back("CV 3");
	m_PluginInfo.PortTips.push_back("CV 4");

	for (int n=0; n<MAX_CHANNELS; n++)
	{
		SetChannel(n, 0.0f);
		SetMin(n, -1.0f);
		SetMax(n, 1.0f);
		SetName(n, "Name");
	}
	
	m_AudioCH->Register("Number",&m_GUIArgs.Number);
	m_AudioCH->Register("Value",&m_GUIArgs.Value);
	m_AudioCH->Register("Min",&m_GUIArgs.Min);
	m_AudioCH->Register("Max",&m_GUIArgs.Max);
	m_AudioCH->RegisterData("Name",ChannelHandler::INPUT,m_GUIArgs.Name,256);
}

ControllerPlugin::~ControllerPlugin()
{
}

PluginInfo &ControllerPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);
}



void ControllerPlugin::Execute()
{
	for(int c=0; c<GetNum(); c++)
	{
		for (int n=0; n<m_HostInfo->BUFSIZE; n++)
		{			
			SetOutput(c,n,GetVal(c));
		}
	}
}

void ControllerPlugin::ExecuteCommands()
{
	if (m_AudioCH->IsCommandWaiting())
	{
		switch (m_AudioCH->GetCommand())
		{
			case (SETNUM)     :
				SetNum(m_GUIArgs.Number);
			break;
			case (SETALL)     :
				SetAll(m_GUIArgs.Number, m_GUIArgs.Name, m_GUIArgs.Max, m_GUIArgs.Value, m_GUIArgs.Min);
			break;
			case (SETNAME)    :
				SetName(m_GUIArgs.Number, m_GUIArgs.Name);
			break;
			case (SETMAX)     :
				SetMax(m_GUIArgs.Number, m_GUIArgs.Max);
			break;
			case (SETCHANNEL) :
				SetChannel(m_GUIArgs.Number, m_GUIArgs.Value);
			break;
			case (SETMIN)     :
				SetMin(m_GUIArgs.Number, m_GUIArgs.Min);
			break;
        }
	}
}

void ControllerPlugin::SetNum(int n)
{
	// once to clear the connections with the current info
	UpdatePluginInfoWithHost();

	if (AsUnsigned(m_NumChannels)<n)
	{
		char t[256];
		sprintf(t,"CV %d",n);
		m_PluginInfo.PortTips.push_back(t);
		AddOutput();
		m_PluginInfo.NumOutputs++;
	}
	else
	{
		vector<string>::iterator i=m_PluginInfo.PortTips.end();
		m_PluginInfo.PortTips.erase(i--);
		RemoveOutput();
		m_PluginInfo.NumOutputs--;
	}
	
	AsUnsigned(m_NumChannels)=n;
	m_PluginInfo.NumOutputs=n;

	// do the actual update
	UpdatePluginInfoWithHost();	
}

// use with caution
void ControllerPlugin::Clear()
{
	m_PluginInfo.PortTips.clear();
	RemoveAllOutputs();
	m_PluginInfo.NumOutputs=0;
}

void ControllerPlugin::Describe(Spiral::Core::Description &d)
{
	d.Value(DeviceVersion).Separator(" ");

	switch (m_Version)
	{
		case 5:
		{
			d.Value(AsUnsigned(m_NumChannels)).Line();
			for (int n=0; n<GetNum(); n++)
			{
				d.Value(GetName(n).size()).Separator(" ");
				d.Value(Name(n)).Separator(" ");
				d.Value(Min(n)).Separator(" ");
				d.Value(Max(n)).Separator(" ");
				d.Value(Val(n)).Line();
			}
		} break;
		case 4:
		{
			d.Value(AsUnsigned(m_NumChannels)).Line();
			for (int n=0; n<GetNum(); n++)
			{
				d.Value(Name(n)).Separator(" ");
				d.Value(Min(n)).Separator(" ");
				d.Value(Max(n)).Separator(" ");
				d.Value(Val(n)).Line();
			}
		} break;
		case 3:
		{
			d.Value(AsUnsigned(m_NumChannels)).Separator(" ");
			for (int n=0; n<GetNum(); n++)
			{
				d.Value(Val(n)).Separator(" ");
			}
			d.Value(1).Line();
			d.Value(AsUnsigned(m_NumChannels)).Separator(" ");
			for (int n=0; n<GetNum(); n++)
			{
				d.Value(GetName(n).size()).Separator(" ");
				d.Value(Name(n)).Separator(" ");
				d.Value(Min(n)).Separator(" ");
				d.Value(Max(n)).Separator(" ");
				d.Value(Val(n)).Line();
			}

		} break;
		default :
			cerr<<"ControllerPlugin - I dont support this streaming version any more"<<endl;
		break;
	}
}

void ControllerPlugin::Apply(Spiral::Core::Description::Reader &r)
{
	int version;
	r.Value(version);

	switch (version)
	{
		case 5:
		{
			Clear();

			r.Value(m_NumChannels);
			string name;
			for (int n=0; n<GetNum(); n++)
			{
				int size,dummy;
				r.Value(size);
				r.Value(Name(n));   // the counted bytes, one value, "" for size 0
				r.Value(Min(n));
				r.Value(Max(n));
				r.Value(Val(n));
			}

		// add the channels one by one
			for (int n=0; n<GetNum(); n++)
			{
				char t[256];
				sprintf(t,"CV %d",n);
				m_PluginInfo.PortTips.push_back(t);
				AddOutput();
			}

			m_PluginInfo.NumOutputs=GetNum();
			UpdatePluginInfoWithHost();
		} break;
		case 4:
		{
			Clear();

			r.Value(m_NumChannels);
			string name;
			for (int n=0; n<GetNum(); n++)
			{
				r.Value(Name(n));
				r.Value(Min(n));
				r.Value(Max(n));
				r.Value(Val(n));
			}

		// add the channels one by one
			for (int n=0; n<GetNum(); n++)
			{
				char t[256];
				sprintf(t,"CV %d",n);
				m_PluginInfo.PortTips.push_back(t);
				AddOutput();
			}

			m_PluginInfo.NumOutputs=GetNum();
			UpdatePluginInfoWithHost();
		} break;
		case 3:
		{
			Clear();

			r.Value(m_NumChannels);

			for (int n=0; n<GetNum(); n++)
			{
				r.Value(Val(n));
			}

			char Buf[4096];
			int size,dummy;
			r.Value(dummy);
			r.Value(m_NumChannels);
			for (int n=0; n<GetNum(); n++)
			{
				r.Value(size);
				r.Value(Name(n));
				r.Value(Min(n));
				r.Value(Max(n));
				r.Value(Val(n));
			}

			// add the channels one by one
			for (int n=0; n<GetNum(); n++)
			{
				char t[256];
				sprintf(t,"CV %d",n);
				m_PluginInfo.PortTips.push_back(t);
				AddOutput();
			}

			m_PluginInfo.NumOutputs=GetNum();
			UpdatePluginInfoWithHost();
		} break;

		default :
			cerr<<"ControllerPlugin - I dont support this streaming version any more"<<endl;
		break;
	}
}
