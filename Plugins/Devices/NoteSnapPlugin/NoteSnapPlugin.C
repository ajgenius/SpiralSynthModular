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
#include "NoteSnapPlugin.h"
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
	return new NoteSnapPlugin;
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
	return 0x0018;
}

string SpiralPlugin_GetName()
{
	return "Note Snap";
}

string SpiralPlugin_GetGroupName()
{
	return "Control";
}
}

///////////////////////////////////////////////////////

static const std::string notes[12]={"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

NoteSnapPlugin::NoteSnapPlugin() :
m_Out(0),
m_LastFreq(-1),
m_LastNote(-1)
{
	for (int n=0; n<12; n++)
	{
		m_Filter[n] = ToggleProperty(Property::WriteOnly, false);
		RegisterSharedProperty(m_Filter[n], notes[n], notes[n]);
	}

	m_PluginInfo.Name="Note Snap";
	m_PluginInfo.Width=80;
	m_PluginInfo.Height=80;
	m_PluginInfo.NumInputs=1;
	m_PluginInfo.NumOutputs=2;
	m_PluginInfo.PortTips.push_back("Input");	
	m_PluginInfo.PortTips.push_back("Output");
	m_PluginInfo.PortTips.push_back("Changed Trigger");
	
	for (int n=0; n<12; n++)
	{
		SetFilter(n, true);
	}

	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 1);
	
}

NoteSnapPlugin::~NoteSnapPlugin()
{
}

PluginInfo &NoteSnapPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);
}



void NoteSnapPlugin::Reset()
{
	ResetPorts();
	m_Out = 0;
	m_LastFreq = -1;
	m_LastNote = -1;
}

void NoteSnapPlugin::Execute()
{	
	float Freq=0;
	
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		Freq=GetInputPitch(0,n);
		
		SetOutput(1,n,0);	
		if (Freq!=m_LastFreq) // if it's changed
		{
			for (int i=0; i<131; i++) // for every note
			{			
				if (GetFilter((i+1)%12) && Freq>=NoteTable[i] && Freq<NoteTable[i+1])
				{		
					m_Out=NoteTable[i];
					if (i!=m_LastNote) 
					{
						SetOutput(1,n,1);
						m_LastNote=i;
					}
				}
			}
		}
		
		m_LastFreq=Freq;
		SetOutputPitch(0,n,m_Out);		
	}
}

void NoteSnapPlugin::ExecuteCommands()
{
}
	
void NoteSnapPlugin::Migrate(Spiral::Core::Description &d)
{
	if (!d.Values().empty())
		return;

	d.Value(1);
	for (int n=0; n<12; n++)
		d.Value(true);
}
