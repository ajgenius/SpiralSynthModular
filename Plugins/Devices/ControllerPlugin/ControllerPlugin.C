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

	m_NumChannels = UnsignedProperty(DefaultLinearFlags | Property::ForceUpdate, 4, 2, 256, 1, 1);
	RegisterSharedProperty(m_NumChannels, "Channels", "Channels");

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

	EnsureChannelProperties();
	
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
			SetOutput(c,n,m_Value[c]->Value.AsFloat);
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
	EnsureChannelProperties();

	// do the actual update
	UpdatePluginInfoWithHost();	
}

// use with caution
void ControllerPlugin::EnsureChannelProperties()
{
	unsigned int c, count = AsUnsigned(m_NumChannels);
	char name[256];

	/* Remove Associated Value Properties */
	while (m_Value.size() > count)
	{
		c = m_Value.size() - 1;
		sprintf(name, "Min%d", c);   UnregisterSharedProperty(name); delete m_Min.back();   m_Min.pop_back();
		sprintf(name, "Max%d", c);   UnregisterSharedProperty(name); delete m_Max.back();   m_Max.pop_back();
		sprintf(name, "Value%d", c); UnregisterSharedProperty(name); delete m_Value.back(); m_Value.pop_back();
		sprintf(name, "Name%d", c);  UnregisterSharedProperty(name); delete m_Name.back();  m_Name.pop_back();
	}

	/* Add Associated Value Properties */
	while ((c = m_Value.size()) < count)
	{
		//Range Minimum
		sprintf(name, "Min%d", c);
		m_Min.push_back(new FloatProperty(DefaultLinearFlags | Property::ForceUpdate, -1.0, -0x7FFFFF, 0x7FFFFF, 0.0001, 0.01));
		RegisterSharedProperty(*m_Min.back(), name, name);

		//Range Maximum
		sprintf(name, "Max%d", c);
		m_Max.push_back(new FloatProperty(DefaultLinearFlags | Property::ForceUpdate, 1.0, -0x7FFFFF, 0x7FFFFF, 0.0001, 0.01));
		RegisterSharedProperty(*m_Max.back(), name, name);

		//Current Value
		/* FIXME - Should be Logarithmic, but as taken as a percentage multiplier */
		sprintf(name, "Value%d", c);
		m_Value.push_back(new FloatProperty(DefaultLinearFlags | Property::ForceUpdate, 0, 0, 100, 0.01, 0.1));
		RegisterSharedProperty(*m_Value.back(), name, name);

		//GUI Specified Name
		/* This Does Not Belong Here - It is a GUI Specific Property - Fix Eventually */
		sprintf(name, "Name%d", c);
		m_Name.push_back(new StringProperty(Property::WriteOnly, "Name"));
		RegisterSharedProperty(*m_Name.back(), name, name);
	}
}

void ControllerPlugin::Clear()
{
	m_PluginInfo.PortTips.clear();
	RemoveAllOutputs();
	m_PluginInfo.NumOutputs=0;
}

void ControllerPlugin::Describe(Spiral::Core::Description &d)
{
	d.Value(m_Version).Separator(" ");

	switch (m_Version)
	{
		case 5:
		{
			d.Value(AsUnsigned(m_NumChannels)).Line();
			for (int n=0; n<GetNum(); n++)
			{
				d.Value(m_Name[n]->Value.size()).Separator(" ");
				d.Value(m_Name[n]->Value).Separator(" ");
				d.Value(m_Min[n]->Value.AsFloat).Separator(" ");
				d.Value(m_Max[n]->Value.AsFloat).Separator(" ");
				d.Value(m_Value[n]->Value.AsFloat).Line();
			}
		} break;
		case 4:
		{
			d.Value(AsUnsigned(m_NumChannels)).Line();
			for (int n=0; n<GetNum(); n++)
			{
				d.Value(m_Name[n]->Value).Separator(" ");
				d.Value(m_Min[n]->Value.AsFloat).Separator(" ");
				d.Value(m_Max[n]->Value.AsFloat).Separator(" ");
				d.Value(m_Value[n]->Value.AsFloat).Line();
			}
		} break;
		case 3:
		{
			d.Value(AsUnsigned(m_NumChannels)).Separator(" ");
			for (int n=0; n<GetNum(); n++)
			{
				d.Value(m_Value[n]->Value.AsFloat).Separator(" ");
			}
			d.Value(1).Line();
			d.Value(AsUnsigned(m_NumChannels)).Separator(" ");
			for (int n=0; n<GetNum(); n++)
			{
				d.Value(m_Name[n]->Value.size()).Separator(" ");
				d.Value(m_Name[n]->Value).Separator(" ");
				d.Value(m_Min[n]->Value.AsFloat).Separator(" ");
				d.Value(m_Max[n]->Value.AsFloat).Separator(" ");
				d.Value(m_Value[n]->Value.AsFloat).Line();
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

			r.Value(AsUnsigned(m_NumChannels));
			EnsureChannelProperties();
			string name;
			for (int n=0; n<GetNum(); n++)
			{
				int size,dummy;
				r.Value(size);
				r.Value(m_Name[n]->Value);   // the counted bytes, one value, "" for size 0
				r.Value(m_Min[n]->Value.AsFloat);
				r.Value(m_Max[n]->Value.AsFloat);
				r.Value(m_Value[n]->Value.AsFloat);
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

			r.Value(AsUnsigned(m_NumChannels));
			EnsureChannelProperties();
			string name;
			for (int n=0; n<GetNum(); n++)
			{
				r.Value(m_Name[n]->Value);
				r.Value(m_Min[n]->Value.AsFloat);
				r.Value(m_Max[n]->Value.AsFloat);
				r.Value(m_Value[n]->Value.AsFloat);
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

			r.Value(AsUnsigned(m_NumChannels));
			EnsureChannelProperties();

			for (int n=0; n<GetNum(); n++)
			{
				r.Value(m_Value[n]->Value.AsFloat);
			}

			char Buf[4096];
			int size,dummy;
			r.Value(dummy);
			r.Value(AsUnsigned(m_NumChannels));
			EnsureChannelProperties();
			for (int n=0; n<GetNum(); n++)
			{
				r.Value(size);
				r.Value(m_Name[n]->Value);
				r.Value(m_Min[n]->Value.AsFloat);
				r.Value(m_Max[n]->Value.AsFloat);
				r.Value(m_Value[n]->Value.AsFloat);
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
