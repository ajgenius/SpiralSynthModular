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

#include "DistributorPlugin.h"
#include "DistributorPluginGUI.h"
#include <FL/Fl_Button.h>
#include "SpiralIcon.xpm"

using namespace std;

extern "C" {

SpiralPlugin* SpiralPlugin_CreateInstance() {
      return new DistributorPlugin;
}

char** SpiralPlugin_GetIcon() {
       return SpiralIcon_xpm;
}

int SpiralPlugin_GetID() {
    return 0x0056;
}

string SpiralPlugin_GetGroupName() {
       return "Control";
}

}

///////////////////////////////////////////////////////

DistributorPlugin::DistributorPlugin():
m_Triggered (false),
m_ChannelSelect (0),
m_TrigDelay (0)
{

	m_PluginInfo.Name = "Distributor";
	m_PluginInfo.Width=80;
	m_PluginInfo.Height=40;

	m_Version = 2;
	m_GUIArgs.ChannelCount = 2;
	CreatePorts ();

	m_AudioCH->Register("ChannelCount", &m_GUIArgs.ChannelCount);
}

DistributorPlugin::~DistributorPlugin() 
{
}

void DistributorPlugin::CreatePorts (int n, bool AddPorts) 
{
	int c;
	char t[256];
	
	m_PluginInfo.NumInputs = 3;
	m_PluginInfo.PortTips.push_back ("Stream");
	m_PluginInfo.PortTips.push_back ("Switcher");
	m_PluginInfo.PortTips.push_back ("Reset CV");

	m_PluginInfo.NumOutputs = n*2;
	for (c=1; c<=n; c++) {
		sprintf (t, "Stream %d", c);
		m_PluginInfo.PortTips.push_back (t);

		sprintf (t, "Switcher %d", c);
		m_PluginInfo.PortTips.push_back (t);
	}

	if (AddPorts) {
		for (int c=0; c<m_PluginInfo.NumInputs; c++) AddInput();
		for (int c=0; c<m_PluginInfo.NumOutputs; c++) AddOutput();
	}
}

void DistributorPlugin::SetChannelCount (int count) 
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


PluginInfo &DistributorPlugin::Initialise (const HostInfo *Host) 
{
           return SpiralPlugin::Initialise (Host);
}

SpiralGUIType *DistributorPlugin::CreateGUI() 
{
	return new DistributorPluginGUI (m_PluginInfo.Width, m_PluginInfo.Height, this, m_AudioCH, m_HostInfo);
}

void DistributorPlugin::Execute() {
	const int Stream = 0;
	const int Switch = 1;
	const int Reset = 2;

	for (int n=0; n<m_HostInfo->BUFSIZE; n++) {
		float InpStream = InputExists (Stream) ? GetInput (Stream, n) : 0.0;
		float InpSwitch = InputExists (Switch) ? GetInput (Switch, n) : 0.0;
		float InpReset = InputExists (Reset) ? GetInput (Reset, n) : 0;
		
		if (InpReset) 
		{
			m_Triggered = false;
			m_ChannelSelect = false;
			m_TrigDelay = 0;			
		}

		if (InpSwitch <= 0.0) 
			m_Triggered = false;
		

		if (! m_Triggered && (InpSwitch > 0.0)) 
		{
			m_Triggered = true;
			SetOutput (m_ChannelSelect+Switch, n, 0);
			m_ChannelSelect = m_ChannelSelect + 2;			
			if (m_ChannelSelect >= (GetChannelCount()*2)) 
				m_ChannelSelect = 0;
				
			m_TrigDelay = 0;
	         }
	         
		SetOutput (m_ChannelSelect+Stream, n, InpStream);

		if (m_TrigDelay < 10) 
		{
			m_TrigDelay++;
			SetOutput (m_ChannelSelect+Switch, n, InpSwitch);
		}
		else 
		{
			SetOutput (m_ChannelSelect+Switch, n, 0);
		}	
	}
}

void DistributorPlugin::ExecuteCommands () 
{
	if (m_AudioCH->IsCommandWaiting ()) 
	{
		switch (m_AudioCH->GetCommand()) {
			case SETCHANNELCOUNT : 
			{
				SetChannelCount(GetChannelCount());
			}	
			break;
		}
	}
}

void DistributorPlugin::StreamOut (ostream &s) 
{
	s << m_Version << " " << GetChannelCount() << " ";
}

void DistributorPlugin::StreamIn (istream &s) 
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
