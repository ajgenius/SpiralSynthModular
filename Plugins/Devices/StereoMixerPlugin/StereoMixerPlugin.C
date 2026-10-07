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
#include "StereoMixerPlugin.h"
#include "Finite.h"
#include "SpiralIcon.xpm"

using namespace std;

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
	return new StereoMixerPlugin;
}

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DSP;
}


const char** SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}

int SpiralPlugin_GetID()
{
	return 0x0008;
}

string SpiralPlugin_GetName()
{
	return "Stereo Mixer";
}

string SpiralPlugin_GetGroupName()
{
	return "Amps/Mixers";
}
}

///////////////////////////////////////////////////////

StereoMixerPlugin::StereoMixerPlugin()
{
	m_PluginInfo.Name="Stereo Mixer";
	m_PluginInfo.Width=190;
	m_PluginInfo.Height=175;
	m_PluginInfo.NumInputs=8;
	m_PluginInfo.NumOutputs=2;
	m_PluginInfo.PortTips.push_back("Input one");	
	m_PluginInfo.PortTips.push_back("Input two");	
	m_PluginInfo.PortTips.push_back("Input three");	
	m_PluginInfo.PortTips.push_back("Input four");	
	m_PluginInfo.PortTips.push_back("Pan CV one");	
	m_PluginInfo.PortTips.push_back("Pan CV two");	
	m_PluginInfo.PortTips.push_back("Pan CV three");	
	m_PluginInfo.PortTips.push_back("Pan CV four");	
	m_PluginInfo.PortTips.push_back("Output left");
	m_PluginInfo.PortTips.push_back("Output right");
	
	for (int n=0; n<NUM_CHANNELS; n++)
	{
		m_ChannelVal[n]=1.0f;
		m_Pan[n]=0.5f;		
	}
	
	m_AudioCH->Register("Num",&m_GUIArgs.Num);
	m_AudioCH->Register("Value",&m_GUIArgs.Value);
}

StereoMixerPlugin::~StereoMixerPlugin()
{
}

PluginInfo &StereoMixerPlugin::Initialise(const HostInfo *Host)
{		
	return SpiralPlugin::Initialise(Host);
}



void StereoMixerPlugin::Execute()
{
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		float left = 0.0f, right = 0.0f;
		for (int c=0; c<NUM_CHANNELS; c++)
		{
			const float volume = m_ChannelVal[c];
			if (volume == 0.0f || !spiralcore::IsFinite(volume)) continue;
			const float in = GetInput(c,n);
			float pan = m_Pan[c];
			if (!spiralcore::IsFinite(in) || !spiralcore::IsFinite(pan)) continue;
			if (InputExists(c))
			{
				const float cv = GetInput(c+4,n);
				// Invalid CV leaves the channel at its base pan.
				if (spiralcore::IsFinite(cv)) pan += cv * 0.5;
			}
			const float channelLeft = (in * volume) * pan;
			const float channelRight = (in * volume) * (1 - pan);
			const float mixedLeft = left + channelLeft;
			const float mixedRight = right + channelRight;
			// Keep both sides together, retaining the other healthy channels.
			if (spiralcore::IsFinite(channelLeft) && spiralcore::IsFinite(channelRight) &&
			    spiralcore::IsFinite(mixedLeft) && spiralcore::IsFinite(mixedRight))
			{
				left = mixedLeft;
				right = mixedRight;
			}
		}
		SetOutput(0,n,left);
		SetOutput(1,n,right);
	}
}

void StereoMixerPlugin::ExecuteCommands()
{
	if (m_AudioCH->IsCommandWaiting())
	{
		switch (m_AudioCH->GetCommand())
		{
			case (SETCH) : SetChannel(m_GUIArgs.Num,m_GUIArgs.Value); break;
			case (SETPAN) : SetPan(m_GUIArgs.Num,m_GUIArgs.Value); break;		
		}
	}
}

void StereoMixerPlugin::StreamOut(ostream &s)
{
	s<<m_Version<<" ";
	for (int n=0; n<NUM_CHANNELS; n++)
	{
		s<<m_ChannelVal[n]<<" "<<m_Pan[n]<<" ";
	}
}

void StereoMixerPlugin::StreamIn(istream &s)
{	
	int version;
	s>>version;
	for (int n=0; n<NUM_CHANNELS; n++)
	{
		s>>m_ChannelVal[n]>>m_Pan[n];
	}
}


