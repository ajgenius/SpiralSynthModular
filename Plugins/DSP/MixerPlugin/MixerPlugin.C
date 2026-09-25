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
	return SPIRAL_PLUGIN_TYPE_DSP;
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

MixerPlugin::MixerPlugin() :
m_NumChannels(4)
{
        int c;
        m_Version = 2;
        m_PluginInfo.Name = "Mixer";
	m_PluginInfo.Width = 80;
	m_PluginInfo.Height = 150;
        for (c=0; c<MAX_CHANNELS; c++) {
            m_ChannelVal[c] = 1.0f;
            m_GUIArgs.inPeak[c] = false;
        }
        m_GUIArgs.Peak = false;
        m_PluginInfo.NumInputs = m_NumChannels;
        m_PluginInfo.NumOutputs = 1;
        for (c=1; c<=m_NumChannels; c++) AddInputTip (c);
        m_PluginInfo.PortTips.push_back ("Output");
	m_AudioCH->Register ("Value", &m_GUIArgs.Value);
	m_AudioCH->Register ("Num", &m_GUIArgs.Num);
	m_AudioCH->Register ("Peak", &m_GUIArgs.Peak, ChannelHandler::OUTPUT);
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
         for (int c=0; c<m_NumChannels; c++) {
             in = 0.0f;
             const float volume = m_ChannelVal[c];
             // Muting must not evaluate NaN * 0 or infinity * 0.
             if (volume != 0.0f && spiralcore::IsFinite(volume)) {
                 const float contribution = GetInput (c, n) * volume;
                 const float mixed = out + contribution;
                 // Reject only this channel, including arithmetic overflow.
                 if (spiralcore::IsFinite(contribution) && spiralcore::IsFinite(mixed)) {
                     in = contribution;
                     out = mixed;
                 }
             }
             m_GUIArgs.inPeak[c] = (in > 1.0);
         }
         SetOutput (0, n, out);
         m_GUIArgs.Peak = (out > 1.0);
     }
}

void MixerPlugin::ExecuteCommands() {
     if (m_AudioCH->IsCommandWaiting()) {
        switch (m_AudioCH->GetCommand()) {
          case SETMIX:
               if (m_GUIArgs.Num >= 0 && m_GUIArgs.Num < m_NumChannels)
                   m_ChannelVal[m_GUIArgs.Num] = m_GUIArgs.Value;
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

void MixerPlugin::SetChannels (int num) {
     // This is only used on loading, so we don't care that it clears all the inputs first
     UpdatePluginInfoWithHost(); // once to clear the connections with the current info
     RemoveAllInputs();
     m_PluginInfo.PortTips.clear ();
     m_PluginInfo.NumInputs = num;
     m_NumChannels = num;
     for (int c=1; c<=m_NumChannels; c++) {
         AddInput ();
         AddInputTip (c);
     }
     m_PluginInfo.PortTips.push_back ("Output");
     UpdatePluginInfoWithHost ();  // do the actual update
}

void MixerPlugin::AddChannel (void) {
     if (m_NumChannels >= MAX_CHANNELS) return;
     m_PluginInfo.NumInputs++;
     m_NumChannels++;
     AddInput ();
     vector<std::string>::iterator i = m_PluginInfo.PortTips.end();
     m_PluginInfo.PortTips.erase (--i);
     AddInputTip (m_NumChannels);
     m_PluginInfo.PortTips.push_back ("Output");
     UpdatePluginInfoWithHost ();  // do the actual update
}

void MixerPlugin::RemoveChannel (void) {
     if (m_NumChannels <= 2) return;
     m_PluginInfo.NumInputs--;
     m_NumChannels--;
     m_PluginInfo.PortTips.pop_back();
     m_PluginInfo.PortTips.pop_back();
     m_PluginInfo.PortTips.push_back ("Output");
     UpdatePluginInfoWithHost (); // disconnect the removed port before discarding its DSP input
     RemoveInput();
}

void MixerPlugin::StreamOut (ostream &s) {
     s << m_Version << " ";
     s << m_NumChannels << " ";
     for (int n=0; n<m_NumChannels; n++) s << m_ChannelVal[n] << " ";
}

void MixerPlugin::StreamIn (istream &s) {
     int version, chans;
     s >> version;
     switch (version) {
       case 1: // needs default number of channels
               break;
       case 2: s >> chans;
               SetChannels (chans);
               break;
     }
     for (int n=0; n<m_NumChannels; n++) s >> m_ChannelVal[n];
}
