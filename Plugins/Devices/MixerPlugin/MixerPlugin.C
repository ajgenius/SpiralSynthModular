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
#include "MixerPlugin.h"
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

    SpiralPlugin* SpiralPlugin_CreateInstance() { return new MixerPlugin; }

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DEVICE;
}

    const char** SpiralPlugin_GetIcon() { return SpiralIcon_xpm; }
    int SpiralPlugin_GetID() { return 0x0007; }
    string SpiralPlugin_GetName()
    {
        return "Mixer";
    }

    string SpiralPlugin_GetGroupName() { return "Amps/Mixers"; }
}

///////////////////////////////////////////////////////

MixerPlugin::MixerPlugin()
{
        int c;
	m_NumChannels = UnsignedProperty(DefaultLinearFlags | Property::ForceUpdate, 4, 2, 256, 1, 1);
	RegisterSharedProperty(m_NumChannels, "Channels", "Channels");

	m_Peak = ToggleProperty(Property::ReadOnly, false);
	RegisterSharedProperty(m_Peak, "Peak", "Peak");

	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 2);

        m_Version = 2;
        m_PluginInfo.Name = "Mixer";
	m_PluginInfo.Width = 80;
	m_PluginInfo.Height = 150;
        for (c=0; c<MAX_CHANNELS; c++) {
            m_GUIArgs.inPeak[c] = false;
        }
        EnsureChannelProperties();
        m_PluginInfo.NumInputs = GetChannels();
        m_PluginInfo.NumOutputs = 1;
        for (c=1; c<=GetChannels(); c++) AddInputTip (c);
        m_PluginInfo.PortTips.push_back ("Output");
	m_AudioCH->Register ("Value", &m_GUIArgs.Value);
	m_AudioCH->Register ("Num", &m_GUIArgs.Num);
	m_AudioCH->RegisterData ("inPeak", ChannelHandler::OUTPUT, m_GUIArgs.inPeak, MAX_CHANNELS * sizeof (bool));
}

MixerPlugin::~MixerPlugin() {
}

void MixerPlugin::AddInputTip (int Channel) {
     char t[256];
     sprintf (t, "Input %d", Channel);
     m_PluginInfo.PortTips.push_back (t);
}

PluginInfo &MixerPlugin::Initialise (const HostInfo *Host) {
    return SpiralPlugin::Initialise (Host);
}



void MixerPlugin::Execute () {
     // Mix the inputs
     for (int n=0; n<m_HostInfo->BUFSIZE; n++) {
         float in, out = 0.0;
         for (int c=0; c<GetChannels(); c++) {
             in = 0.0f;
             const float volume = m_Volume[c]->Value.AsFloat;
             // Muting must not evaluate NaN * 0 or infinity * 0.
             if (volume != 0.0f && Spiral::Core::IsFinite(volume)) {
                 const float contribution = GetInput (c, n) * volume;
                 const float mixed = out + contribution;
                 // Reject only this channel, including arithmetic overflow.
                 if (Spiral::Core::IsFinite(contribution) && Spiral::Core::IsFinite(mixed)) {
                     in = contribution;
                     out = mixed;
                 }
             }
             m_GUIArgs.inPeak[c] = (in > 1.0);
             m_VolumePeak[c]->Value.AsBoolean = (in > 1.0);
         }
         SetOutput (0, n, out);
         AsBoolean(m_Peak) = (out > 1.0);
     }
}

void MixerPlugin::ExecuteCommands() {
     if (m_AudioCH->IsCommandWaiting()) {
        switch (m_AudioCH->GetCommand()) {
          case SETMIX:
               if (m_GUIArgs.Num >= 0 && m_GUIArgs.Num < GetChannels())
                   m_Volume[m_GUIArgs.Num]->Value.AsFloat = m_GUIArgs.Value;
               break;
          case ADDCHAN:
               AddChannel ();
               break;
          case REMOVECHAN:
               RemoveChannel ();
               break;
        }
      }
}

void MixerPlugin::EnsureChannelProperties()
{
	unsigned int num = AsUnsigned(m_NumChannels);
	char name[256];

	/* Remove Associated Value Properties */
	while( m_Volume.size() > num) 
	{
		UnregisterSharedProperty(m_Volume.back()->GetLabel());
		delete m_Volume.back();
		m_Volume.pop_back();

		UnregisterSharedProperty(m_VolumePeak.back()->GetLabel());
		delete m_VolumePeak.back();
		m_VolumePeak.pop_back();
	}

	/* Add Associated Value Properties */
	while (m_Volume.size() < num) 
	{
		sprintf(name, "Volume%d", (int)m_Volume.size());
		m_Volume.push_back(new FloatProperty(DefaultLinearFlags, 1.0, 0.0, 2.0, 0.001, 0.01));
		RegisterSharedProperty(*m_Volume.back(), name, name);

		sprintf(name, "Peak%d", (int)m_VolumePeak.size());
		m_VolumePeak.push_back(new ToggleProperty(Property::ReadOnly, false));
		RegisterSharedProperty(*m_VolumePeak.back(), name, name);
	}
}

void MixerPlugin::SetChannels (int num) {
     num = std::max(2, std::min(MAX_CHANNELS, num));
     // This is only used on loading, so we don't care that it clears all the inputs first
     UpdatePluginInfoWithHost(); // once to clear the connections with the current info
     RemoveAllInputs();
     m_PluginInfo.PortTips.clear ();
     m_PluginInfo.NumInputs = num;
     AsUnsigned(m_NumChannels) = num;
     EnsureChannelProperties();
     for (int c=1; c<=GetChannels(); c++) {
         AddInput ();
         AddInputTip (c);
     }
     m_PluginInfo.PortTips.push_back ("Output");
     UpdatePluginInfoWithHost ();  // do the actual update
}

void MixerPlugin::AddChannel (void) {
     if (GetChannels() >= MAX_CHANNELS) return;
     m_PluginInfo.NumInputs++;
     AsUnsigned(m_NumChannels)++;
     EnsureChannelProperties();
     AddInput ();
     vector<std::string>::iterator i = m_PluginInfo.PortTips.end();
     m_PluginInfo.PortTips.erase (--i);
     AddInputTip (GetChannels());
     m_PluginInfo.PortTips.push_back ("Output");
     UpdatePluginInfoWithHost ();  // do the actual update
}

void MixerPlugin::RemoveChannel (void) {
     if (GetChannels() <= 2) return;
     m_PluginInfo.NumInputs--;
     AsUnsigned(m_NumChannels)--;
     EnsureChannelProperties();
     m_PluginInfo.PortTips.pop_back();
     m_PluginInfo.PortTips.pop_back();
     m_PluginInfo.PortTips.push_back ("Output");
     UpdatePluginInfoWithHost (); // disconnect the removed port before discarding its DSP input
     RemoveInput();
}

void MixerPlugin::Describe(Spiral::Core::Description &d) {
     d.Value(m_Version).Separator(" ");
     d.Value(AsUnsigned(m_NumChannels)).Separator(" ");
     for (int n=0; n<GetChannels(); n++) d.Value(m_Volume[n]->Value.AsFloat).Separator(" ");
}

void MixerPlugin::Apply(Spiral::Core::Description::Reader &r) {
     int version = 0, chans = GetChannels();
     r.Value(version);
     switch (version) {
       case 1: // needs default number of channels
               break;
       case 2: r.Value(chans);
               SetChannels (chans);
               break;
     }
     // Consume old oversized records without writing beyond the fixed gains.
     for (int n=0; n<chans && !r.Failed(); n++) {
          float value = 1.0f;
          r.Value(value);
          if (n<GetChannels()) m_Volume[n]->Value.AsFloat = value;
     }
}
