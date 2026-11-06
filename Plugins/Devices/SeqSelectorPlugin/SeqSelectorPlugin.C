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
#include <cstring>
#include "SeqSelectorPlugin.h"
#include "SpiralIcon.xpm"
#include "NoteTable.h"
#include <vector>
#include <algorithm>

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
}

///////////////////////////////////////////////////////

VectorProperty *SeqSelectorPlugin::NewLine()
{
	VectorProperty *Line = new VectorProperty(Property::WriteOnly);

	for (int n=0; n<NUM_VALUES; n++)
		Line->Add(new SignedProperty(DefaultLinearFlags, 0, -1, 131, 1, 1));

	return Line;
}

SeqSelectorPlugin::SeqSelectorPlugin()
{
	m_Lines = VectorProperty(Property::WriteOnly);
	RegisterSharedProperty(m_Lines, "Lines", "The sequence, line by line");

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
	
	m_Triggered=false;

	SetUnsigned(DeviceVersion, 2);
	
	m_Begin = SignedProperty(DefaultLinearFlags, 0, 0, 255, 1, 1);
	RegisterSharedProperty(m_Begin, "Begin", "First line of the range");
	m_End = SignedProperty(DefaultLinearFlags, 0, 0, 255, 1, 1);
	RegisterSharedProperty(m_End, "End", "Line after the last of the range");
	m_UseRange = ToggleProperty(Property::WriteOnly, false);
	RegisterSharedProperty(m_UseRange, "UseRange", "Play the range, not every line");
	m_Pos = UnsignedProperty(Property::ReadOnly, 0, 0, 255, 1, 1);
	RegisterSharedProperty(m_Pos, "Position", "The line playing");
	m_LineCount = UnsignedProperty(Property::ReadOnly, 0, 0, 255, 1, 1);
	RegisterSharedProperty(m_LineCount, "Line Count", "Lines in the sequence");

	m_AudioCH->Register("Num",&m_GUIArgs.Num);
	m_AudioCH->Register("Line",&m_GUIArgs.Line);
	m_AudioCH->Register("Val",&m_GUIArgs.Val);
	m_AudioCH->RegisterData("Lines",ChannelHandler::OUTPUT,m_EchoValues,sizeof(m_EchoValues));
	std::vector<std::string> none, cell;
	cell.push_back("Line"); cell.push_back("Num"); cell.push_back("Val");
	m_AudioCH->RegisterCommand("ADD_LINE", ADD_LINE, none);
	m_AudioCH->RegisterCommand("REM_LINE", REM_LINE, none);
	m_AudioCH->RegisterCommand("SET_VAL", SET_VAL, cell);
	RefreshEcho();
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
	AsUnsigned(m_Pos)=0;
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
				AsUnsigned(m_Pos)++;
				
				if (AsBoolean(m_UseRange))
				{
					if (AsUnsigned(m_Pos)>=(unsigned)AsSigned(m_End))
					{
						AsUnsigned(m_Pos)=AsSigned(m_Begin);
					}
				}
				else
				{
					if (AsUnsigned(m_Pos)>=m_Lines.size())
					{
						AsUnsigned(m_Pos)=0;
					}
				}
								
				for(int i=0; i<8; i++)
				{
					const int value = LineValue(AsUnsigned(m_Pos), i);
					SetOutputPitch(i,n,value < 0 || value > 131 ? -1 : NoteTable[value]);
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

void SeqSelectorPlugin::RefreshEcho()
{
	AsUnsigned(m_LineCount) = m_Lines.size();
	memset(m_EchoValues, 0, sizeof(m_EchoValues));
	for (unsigned l = 0; l < m_Lines.size() && l < MAX_ECHO_LINES; ++l)
		for (int n = 0; n < NUM_VALUES; ++n)
			m_EchoValues[l * NUM_VALUES + n] = LineValue(l, n);
}

void SeqSelectorPlugin::ExecuteCommands()
{
	if (m_AudioCH->IsCommandWaiting())
	{
		switch (m_AudioCH->GetCommand())
		{
			case ADD_LINE  : 
				{
					VectorProperty *Added = NewLine();
					if (m_Lines.size()) 
					{
						for (int n=0; n<NUM_VALUES; n++) ((SignedProperty *)(*Added)[n])->Value.AsSigned = LineValue(m_Lines.size()-1, n);
					}
					else 
					{
						for (int n=0; n<NUM_VALUES; n++) ((SignedProperty *)(*Added)[n])->Value.AsSigned = 0;
					}
					m_Lines.Add(Added);
				} break;
			case REM_LINE  : m_Lines.RemoveLast(); break;
			case SET_VAL   : SetLineValue(m_GUIArgs.Line, m_GUIArgs.Num, m_GUIArgs.Val); break;
			default: break;
		}
		RefreshEcho();
	}
}

void SeqSelectorPlugin::Describe(Spiral::Core::Description &d)
{
	d.Value(DeviceVersion).Separator(" ");
	d.Value(m_Lines.size()).Separator(" ");
	
	for (UnsignedType line=0; line<m_Lines.size(); line++)
	{
		for (int n=0; n<NUM_VALUES; n++)
		{
			d.Value(Cell(line, n)).Separator(" ");
		}
	}
	d.Value(m_Begin).Separator(" ").Value(m_End).Separator(" ").Value(m_UseRange);
}

void SeqSelectorPlugin::Apply(Spiral::Core::Description::Reader &r)
{	
	int version;
	r.Value(version);
	
	std::vector<int> kept;
	for (UnsignedType line=0; line<m_Lines.size(); line++)
		for (int n=0; n<NUM_VALUES; n++)
			kept.push_back(LineValue(line, n));
	while (m_Lines.size()) m_Lines.RemoveLast();

	int Num;
	r.Count(m_Lines, Num);
	
	bool valid = Num <= 255;
	for (int i=0; i<Num && r.More() && (valid || !r.Named()); i++)
	{
		m_Lines.Add(NewLine());
		for (int n=0; n<NUM_VALUES; n++)
		{
			r.Value(Cell(i, n));
			valid = valid && LineValue(i, n) >= -1 && LineValue(i, n) <= 131;
		}
	}	
	if (!valid && r.Named())
	{
		while (m_Lines.size()) m_Lines.RemoveLast();
		for (size_t i=0; i<kept.size(); i++)
		{
			if (i % NUM_VALUES == 0) m_Lines.Add(NewLine());
			SetLineValue(i / NUM_VALUES, i % NUM_VALUES, kept[i]);
		}
	}
	else if (!valid)
	{
		while (m_Lines.size() > 255) m_Lines.RemoveLast();
		for (UnsignedType line=0; line<m_Lines.size(); line++)
			for (int n=0; n<NUM_VALUES; n++)
				SetLineValue(line, n, std::max(-1, std::min(131, LineValue(line, n))));
	}
	r.Value(m_Begin).Value(m_End).Value(m_UseRange);
	RefreshEcho();
}

void SeqSelectorPlugin::Migrate(Spiral::Core::Description &d)
{
	if (d.Version() != 1)
		return;

	d.Value(0).Value(0).Value(false);
	d.SetValue(0, 2);
}
