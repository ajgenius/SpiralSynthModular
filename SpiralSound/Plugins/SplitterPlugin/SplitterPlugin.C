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
#include <string>
#include <iostream>
#include <stdio.h>

#include "SplitterPlugin.h"
#include "SplitterPluginGUI.h"
#include <FL/Fl_Button.h>
#include "SpiralIcon.xpm"

using namespace std;

extern "C" {
SpiralPlugin* SpiralPlugin_CreateInstance()
{
	return new SplitterPlugin;
}

char** SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}

int SpiralPlugin_GetID()
{
	return 0x0006;
}

string SpiralPlugin_GetGroupName()
{
	return "Control";
}
}

///////////////////////////////////////////////////////

SplitterPlugin::SplitterPlugin()
{
	m_PluginInfo.Name="Splitter";
	m_PluginInfo.Width=80;
	m_PluginInfo.Height=40;

	m_Version = 2;
	m_GUIArgs.ChannelCount = 4;
	CreatePorts ();

	m_AudioCH->Register ("ChannelCount", &m_GUIArgs.ChannelCount);
}

SplitterPlugin::~SplitterPlugin()
{
}

void SplitterPlugin::CreatePorts (int n, bool AddPorts) 
{
	int c;
	char t[256];
	
	m_PluginInfo.NumInputs = 1;
	m_PluginInfo.PortTips.push_back ("Input");

	m_PluginInfo.NumOutputs = n;
	for (c=1; c<=n; c++) {
		sprintf (t, "Out %d", c);
		m_PluginInfo.PortTips.push_back (t);
	}

	if (AddPorts) {
		for (int c=0; c<m_PluginInfo.NumInputs; c++) AddInput();
		for (int c=0; c<m_PluginInfo.NumOutputs; c++) AddOutput();
	}
}

void SplitterPlugin::SetChannelCount (int count) 
{
	m_GUIArgs.ChannelCount = count;

	UpdatePluginInfoWithHost();

	RemoveAllInputs ();
	RemoveAllOutputs ();
	m_PluginInfo.NumInputs = 0;
	m_PluginInfo.NumOutputs = 0;
	m_PluginInfo.PortTips.clear ();

	CreatePorts (count, true);
	UpdatePluginInfoWithHost ();
}

PluginInfo &SplitterPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise( Host);
}

SpiralGUIType *SplitterPlugin::CreateGUI()
{
	return new SplitterPluginGUI (m_PluginInfo.Width, m_PluginInfo.Height, this, m_AudioCH, m_HostInfo);
}

void SplitterPlugin::Execute()
{
	// Just copy the data through.
	for (int i=0; i<GetChannelCount(); i++)
	{
		GetOutputBuf(i)->Zero();
	}
	
	if (InputExists(0))  
	{
		for (int i=0; i<GetChannelCount(); i++)
		{
			GetOutputBuf(i)->Mix(*GetInput(0),0);
		}	
	}
}

void SplitterPlugin::ExecuteCommands () 
{
	if (m_AudioCH->IsCommandWaiting ()) 
	{
		switch (m_AudioCH->GetCommand()) {
			case SETCHANNELCOUNT : 
			{
				UpdatePluginInfoWithHost();

				RemoveAllInputs ();
				RemoveAllOutputs ();
				m_PluginInfo.NumInputs = 0;
				m_PluginInfo.NumOutputs = 0;
				m_PluginInfo.PortTips.clear ();

				CreatePorts (m_GUIArgs.ChannelCount, true);
				UpdatePluginInfoWithHost ();
			}
			break;
		}
	}
}

void SplitterPlugin::StreamOut (ostream &s) 
{
	s << m_Version << " " << GetChannelCount() << " ";
}

void SplitterPlugin::StreamIn (istream &s) 
{
	char Test;
	int Version, Channels;

	s.seekg (2, ios_base::cur );//skip to next line
	Test = s.peek();//peek first char
	s.seekg (-2, ios_base::cur );//jump back to prior line 
	
	if ( (Test >= '0') && (Test <= '9') )
	{
		s >> Version;
	}
	else
	{
		//No Version, so use Version 1
		Version = 1;
	}
	
	switch (Version)
	{
		case 2:
		{
			s >> Channels;
			SetChannelCount (Channels);
		}
		break;
		
		case 1:
		{
			//use original fixed defaults
			SetChannelCount (4);
		}
		break;
	}	
}
