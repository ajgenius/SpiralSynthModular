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
#include "MixerPluginGUI.h"
#include <FL/Fl_Button.H>
#include "SpiralIcon.xpm"

using namespace std;

extern "C" {
    SpiralPlugin* SpiralPlugin_CreateInstance() { return new MixerPlugin; }
    char** SpiralPlugin_GetIcon() { return SpiralIcon_xpm; }
    int SpiralPlugin_GetID() { return 0x0007; }
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

SpiralGUIType *MixerPlugin::CreateGUI() {
    return new MixerPluginGUI (m_PluginInfo.Width, m_PluginInfo.Height, this, m_AudioCH, m_HostInfo);
}

void MixerPlugin::Execute () {
     // Mix the inputs
     for (int n=0; n<m_HostInfo->BUFSIZE; n++) {
         float in, out = 0.0;
         for (int c=0; c<m_NumChannels; c++) {
             in = GetInput (c, n) * m_ChannelVal[c];
             m_GUIArgs.inPeak[c] = (in > 1.0);
             out += in;
         }
         SetOutput (0, n, out);
         m_GUIArgs.Peak = (out > 1.0);
     }
}

void MixerPlugin::ExecuteCommands() {
     if (m_AudioCH->IsCommandWaiting()) {
        switch (m_AudioCH->GetCommand()) {
          case SETMIX:
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
     UpdatePluginInfoWithHost(); // once to clear the connections with the current info
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
     UpdatePluginInfoWithHost(); // once to clear the connections with the current info
     m_PluginInfo.NumInputs--;
     m_NumChannels--;
     vector<std::string>::iterator i = m_PluginInfo.PortTips.end();
     m_PluginInfo.PortTips.erase (--i);
     m_PluginInfo.PortTips.erase (--i);
     m_PluginInfo.PortTips.push_back ("Output");
     RemoveInput();
     UpdatePluginInfoWithHost ();  // do the actual update
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
