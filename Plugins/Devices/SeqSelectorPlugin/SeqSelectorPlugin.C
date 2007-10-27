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
#include "SeqSelectorPlugin.h"
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
	return new SeqSelectorPlugin;
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
	return 0x0015;
}

string SpiralPlugin_GetName()
{
	return "SeqSelector";
}

string SpiralPlugin_GetGroupName()
{
	return "Sequencing";
}
}

///////////////////////////////////////////////////////

SeqSelectorPlugin::SeqSelectorPlugin()
{
	m_PluginInfo.Name="SeqSelector";
	m_PluginInfo.Width=300;
	m_PluginInfo.Height=200;
	m_PluginInfo.NumInputs=1;
	m_PluginInfo.NumOutputs=8;
	m_PluginInfo.PortTips.push_back("Trigger");	
	m_PluginInfo.PortTips.push_back("CV One");	
	m_PluginInfo.PortTips.push_back("CV Two");	
	m_PluginInfo.PortTips.push_back("CV Three");	
	m_PluginInfo.PortTips.push_back("CV Four");	
	m_PluginInfo.PortTips.push_back("CV Five");	
	m_PluginInfo.PortTips.push_back("CV Six");	
	m_PluginInfo.PortTips.push_back("CV Seven");	
	m_PluginInfo.PortTips.push_back("CV Eight");	
	
	m_NumLines = UnsignedProperty(DefaultLinearFlags | Property::ForceUpdate, 0, 0, 255, 1, 1);
	m_Begin = SignedProperty(DefaultLinearFlags, 0, 0, 255, 1, 1);
	m_End = SignedProperty(DefaultLinearFlags, 0, 0, 255, 1, 1);
	m_Position = UnsignedProperty(DefaultLinearFlags, 0, 0, 255, 1, 1);

	m_UseRange = ToggleProperty(Property::WriteOnly, false);
	RegisterSharedProperty(m_UseRange, "UseRange", "UseRange");

	RegisterSharedProperty(m_NumLines, "Lines", "Lines");
	RegisterSharedProperty(m_Begin, "Begin", "Begin");	
	RegisterSharedProperty(m_End, "End", "End");
	RegisterSharedProperty(m_Position, "Position", "Position");

	m_Triggered=false;

	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 1);
	
	m_AudioCH->Register("Num",&m_GUIArgs.Num);
	m_AudioCH->Register("Line",&m_GUIArgs.Line);
	m_AudioCH->Register("Val",&m_GUIArgs.Val);
}

SeqSelectorPlugin::~SeqSelectorPlugin()
{
}

PluginInfo &SeqSelectorPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);
}



void SeqSelectorPlugin::Reset()
{
	ResetPorts();
	AsUnsigned(m_Position)=0;
	m_Triggered=false;
	AsBoolean(m_UseRange)=false;	
}

void SeqSelectorPlugin::Execute()
{
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		// Sends momentary spike of value when triggered, 
		// works on the note off, rather than the first
		// detection of a trigger value
		if (GetInput(0,n)>0.1)
		{
			m_Triggered=true;
		}
		else
		{			
			if (m_Triggered==true && m_Lines.size()>0)			
			{								
				AsUnsigned(m_Position)++;
				
				if (AsBoolean(m_UseRange))
				{
					if (AsUnsigned(m_Position)>=(unsigned)AsSigned(m_End))
					{
						AsUnsigned(m_Position)=AsSigned(m_Begin);
					}
				}
				else
				{
					if (AsUnsigned(m_Position)>=m_Lines.size())
					{
						AsUnsigned(m_Position)=0;
					}
				}
								
				for(int i=0; i<8; i++)
				{			
					SetOutputPitch(i,n,NoteTable[m_Lines[AsUnsigned(m_Position)].Value[i]->Value.AsSigned]);
				}														
			}					
			else
			{
				for(int i=0; i<8; i++)
				{
					// zero frequency
					SetOutputPitch(i,n,-1);		
				}
			}
			
			m_Triggered=false;						
		}		
	}
}

void SeqSelectorPlugin::ExecuteCommands()
{
	if (m_AudioCH->IsCommandWaiting())
	{
		switch (m_AudioCH->GetCommand())
		{
			case SET_BEGIN : AsSigned(m_Begin)=m_GUIArgs.Line; break;
			case SET_END   : AsSigned(m_End)=m_GUIArgs.Line; break;
			case RANGE     : AsBoolean(m_UseRange)=m_GUIArgs.Val; break;
			case ADD_LINE  : UpdateLineCount(m_Lines.size()+1); break;
			case REM_LINE  : UpdateLineCount(m_Lines.size()-1); break;
			case SET_VAL   : m_Lines[m_GUIArgs.Line].Value[m_GUIArgs.Num]->Value.AsSigned=m_GUIArgs.Val; break;
			default: break;
		}
	}
}

void SeqSelectorPlugin::UpdateLineCount(unsigned int num)
{
	char name[256];
	unsigned int c;
	
	AsUnsigned(m_NumLines) = num;

	/* Remove Values */
	while (m_Lines.size() > num)
	{
		c = m_Lines.size() - 1;
		for (int n=0; n<NUM_VALUES; n++)
		{
			sprintf(name, "Line%uValue%u", c, n);
			UnregisterSharedProperty(name);
			delete m_Lines.back().Value[n];
		}
		m_Lines.pop_back();
	}

	/* Add Values */
	while ((c = m_Lines.size()) < num)
	{
		m_Lines.push_back(Line());
		for (int n=0; n<NUM_VALUES; n++)
		{
			m_Lines[c].Value.push_back(new SignedProperty(DefaultLinearFlags, (c<1)?0:m_Lines[c-1].Value[n]->Value.AsSigned, -1, 15, 1, 1));
			sprintf(name, "Line%uValue%u", c, n);
			RegisterSharedProperty(*m_Lines.back().Value.back(), name, name);
		}
	}
}

void SeqSelectorPlugin::Describe(Spiral::Core::Description &d)
{
	d.Value(m_Version).Separator(" ");
	d.Value(m_Lines.size()).Separator(" ");
	
	if (m_Lines.size()>0)
	{
		for (vector<Line>::iterator i = m_Lines.begin();
			 i!=m_Lines.end(); i++)
		{
			for (int n=0; n<NUM_VALUES; n++)
			{
				d.Value(i->Value[n]->Value.AsSigned).Separator(" ");
			}
		}
	}
}

void SeqSelectorPlugin::Apply(Spiral::Core::Description::Reader &r)
{	
	int version;
	r.Value(version);
	
	int Num;
	r.Value(Num);
	
	UpdateLineCount(Num);
	for (int i=0; i<Num; i++)
	{
		for (int n=0; n<NUM_VALUES; n++)
		{
			r.Value(m_Lines[i].Value[n]->Value.AsSigned);
		}
	}	
}
