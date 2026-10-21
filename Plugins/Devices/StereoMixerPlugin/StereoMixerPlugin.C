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
#include <algorithm>
#include <stdio.h>
#include "Finite.h"
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
	return new StereoMixerPlugin;
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

static const char *Numbers[MAX_CHANNELS] = {"one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten"};

StereoMixerPlugin::StereoMixerPlugin() :
m_NumChannels(4),
m_MovedFrom(4)
{
	m_ChannelVal = VectorProperty(Property::WriteOnly);
	for (int n=0; n<MAX_CHANNELS; n++) m_ChannelVal.Add(new FloatProperty(DefaultLinearFlags, 1.0f, 0.0f, 2.0f, 0.001f, 0.01f));
	m_Pan = VectorProperty(Property::WriteOnly);
	for (int n=0; n<MAX_CHANNELS; n++) m_Pan.Add(new FloatProperty(DefaultLinearFlags, 0.5f, 0.0f, 1.0f, 0.001f, 0.01f));
	RegisterSharedProperty(m_ChannelVal, "ChannelVal", "Channel volumes");
	RegisterSharedProperty(m_Pan, "Pan", "Channel pans");
	for (int n=0; n<MAX_CHANNELS; n++)
	{
		char key[16], hint[32];
		snprintf(key, sizeof key, "Volume%d", n);
		snprintf(hint, sizeof hint, "Channel %d volume", n + 1);
		RegisterSharedProperty(Channel(n), key, hint);
		snprintf(key, sizeof key, "Pan%d", n);
		snprintf(hint, sizeof hint, "Channel %d pan", n + 1);
		RegisterSharedProperty(Pan(n), key, hint);
	}
	m_Channels = UnsignedProperty(DefaultLinearFlags, m_NumChannels, 2, MAX_CHANNELS, 1, 1);
	RegisterSharedProperty(m_Channels, "Channels", "Number of channels");

	m_PluginInfo.Name="Stereo Mixer";
	m_PluginInfo.Width=190;
	m_PluginInfo.Height=175;
	m_PluginInfo.NumInputs=m_NumChannels*2;
	m_PluginInfo.NumOutputs=2;
	for (int n=0; n<m_NumChannels; n++) m_PluginInfo.PortTips.push_back(string("Input ")+Numbers[n]);
	for (int n=0; n<m_NumChannels; n++) m_PluginInfo.PortTips.push_back(string("Pan CV ")+Numbers[n]);
	m_PluginInfo.PortTips.push_back("Output left");
	m_PluginInfo.PortTips.push_back("Output right");
	
	SetString(DeviceName, SpiralPlugin_GetName());

	for (int n=0; n<MAX_CHANNELS; n++)
	{
		SetChannel(n, 1.0f);
		SetPan(n, 0.5f);		
	}
	
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
		for (int c=0; c<m_NumChannels; c++)
		{
			const float volume = GetChannel(c);
			if (volume == 0.0f || !Spiral::Core::IsFinite(volume)) continue;
			const float in = GetInput(c,n);
			float pan = GetPan(c);
			if (!Spiral::Core::IsFinite(in) || !Spiral::Core::IsFinite(pan)) continue;
			if (InputExists(c))
			{
				const float cv = GetInput(c+m_NumChannels,n);
				// Invalid CV leaves the channel at its base pan.
				if (Spiral::Core::IsFinite(cv)) pan += cv * 0.5;
			}
			const float channelLeft = (in * volume) * pan;
			const float channelRight = (in * volume) * (1 - pan);
			const float mixedLeft = left + channelLeft;
			const float mixedRight = right + channelRight;
			// Keep both sides together, retaining the other healthy channels.
			if (Spiral::Core::IsFinite(channelLeft) && Spiral::Core::IsFinite(channelRight) &&
			    Spiral::Core::IsFinite(mixedLeft) && Spiral::Core::IsFinite(mixedRight))
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
	if ((int)AsUnsigned(m_Channels) != m_NumChannels)
		SetChannels(AsUnsigned(m_Channels));
}

void StereoMixerPlugin::SetChannels(int num)
{
	num = std::max(2, std::min(MAX_CHANNELS, num));
	// Fewer channels stop at the last one wired, so no wire is dropped.
	for (int c=m_NumChannels-1; c>=num; c--)
	{
		if (InputExists(c) || InputExists(m_NumChannels+c))
		{
			num = c+1;
			break;
		}
	}
	if (num == m_NumChannels)
	{
		AsUnsigned(m_Channels) = m_NumChannels;
		return;
	}

	// The host moves the pan wires along with the audio inputs (MovedInput),
	// then the inputs themselves move, so every wire keeps its port.
	m_MovedFrom = m_NumChannels;
	m_NumChannels = num;
	m_PluginInfo.NumInputs = num*2;
	m_PluginInfo.PortTips.clear();
	for (int n=0; n<num; n++) m_PluginInfo.PortTips.push_back(string("Input ")+Numbers[n]);
	for (int n=0; n<num; n++) m_PluginInfo.PortTips.push_back(string("Pan CV ")+Numbers[n]);
	m_PluginInfo.PortTips.push_back("Output left");
	m_PluginInfo.PortTips.push_back("Output right");
	UpdatePluginInfoWithHost();

	const int was = m_MovedFrom;
	vector<const Sample*> held;
	for (int n=0; n<was*2; n++) held.push_back(GetInput(n));
	RemoveAllInputs();
	for (int n=0; n<num*2; n++) AddInput();
	for (int c=0; c<std::min(was, num); c++)
	{
		SetInput(c, held[c]);
		SetInput(num+c, held[was+c]);
	}
	m_MovedFrom = m_NumChannels;
	AsUnsigned(m_Channels) = m_NumChannels;
}

int StereoMixerPlugin::MovedInput(int port) const
{
	if (port < m_MovedFrom) return port < m_NumChannels ? port : -1;
	port -= m_MovedFrom;
	return port < m_NumChannels ? m_NumChannels+port : -1;
}

void StereoMixerPlugin::Describe(Spiral::Core::Description &d)
{
	d.Value(DeviceVersion).Separator(" ");
	AsUnsigned(m_Channels) = m_NumChannels;
	d.Value(m_Channels).Separator(" ");
	for (int n=0; n<MAX_CHANNELS; n++)
	{
		d.Value(Channel(n)).Separator(" ").Value(Pan(n)).Separator(" ");
	}
}

void StereoMixerPlugin::Apply(Spiral::Core::Description::Reader &r)
{	
	int version;
	r.Value(version);
	int saved = 4;
	if (r.Find("CHANNELS") >= 0)
	{
		r.Value(m_Channels);
		saved = MAX_CHANNELS;
		SetChannels(AsUnsigned(m_Channels));
	}
	for (int n=0; n<saved; n++)
	{
		r.Value(Channel(n)).Value(Pan(n));
	}
}


