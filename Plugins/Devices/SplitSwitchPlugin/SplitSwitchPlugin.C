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
#include "SplitSwitchPlugin.h"
#include "Finite.h"
#include <stdio.h>
#include <math.h>
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


   SpiralPlugin* SpiralPlugin_CreateInstance () { return new SplitSwitchPlugin; }

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DEVICE;
}


   const char** SpiralPlugin_GetIcon () { return SpiralIcon_xpm; }

   int SpiralPlugin_GetID () { return 125; }

   string SpiralPlugin_GetName()
   {
       return "SplitSwitch";
   }

   string SpiralPlugin_GetGroupName() { return "Maths/Logic"; }
}

///////////////////////////////////////////////////////

SplitSwitchPlugin::SplitSwitchPlugin () :
m_Triggered (false)
{
	m_SwitchPos = SignedProperty(DefaultLinearFlags, 0, 0, 100, 1, 1);
  m_Chans = SignedProperty(DefaultLinearFlags, 2, 1, 256, 1, 1);
  RegisterSharedProperty(m_Chans, "Chans", "Number of channels");
  RegisterSharedProperty(m_SwitchPos, "SwitchPos", "Selected output");
  m_GUIArgs.Switch = 1;
  m_GUIArgs.Echo = 1;
  m_PluginInfo.Name = "SplitSwitch";
  m_PluginInfo.Width = 80;
  m_PluginInfo.Height = 80;
  m_PluginInfo.NumInputs = 3;
  m_PluginInfo.NumOutputs = 3;
  m_PluginInfo.PortTips.push_back ("CV");
  m_PluginInfo.PortTips.push_back ("Clock");
  m_PluginInfo.PortTips.push_back ("In");
  m_PluginInfo.PortTips.push_back ("CV");
  m_PluginInfo.PortTips.push_back ("Out 1");
  m_PluginInfo.PortTips.push_back ("Out 2");

	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 1);
  // Switch Position <- GUI
  m_AudioCH->Register ("Switch", &m_GUIArgs.Switch);
  // Switch Position -> GUI
  m_AudioCH->Register ("Echo", &m_GUIArgs.Echo, ChannelHandler::OUTPUT);
  // Auto Mode (Switch position is controlled by CV or Clock = True, Switch Position controlled by GUI = False
  m_AudioCH->Register ("Auto", &m_GUIArgs.Auto, ChannelHandler::OUTPUT);
}

SplitSwitchPlugin::~SplitSwitchPlugin () {
}

PluginInfo &SplitSwitchPlugin::Initialise (const HostInfo *Host) {
  return SpiralPlugin::Initialise (Host);
}



void SplitSwitchPlugin::ExecuteCommands () {
  if (m_AudioCH->IsCommandWaiting ()) {
    switch (m_AudioCH->GetCommand()) {
      case (SETCHANS)  : SetChans (AsSigned(m_Chans));
                         break;
    }
  }
}

void SplitSwitchPlugin::SetChans (int n) {
  // once to clear the connections with the current info
  UpdatePluginInfoWithHost();
  while (n > m_PluginInfo.NumOutputs - 1) {
    m_PluginInfo.NumOutputs++;
    char t[256];
    sprintf (t, "Out %d", n);
    m_PluginInfo.PortTips.push_back (t);
    AddOutput ();
  }
  while (n < m_PluginInfo.NumOutputs - 1) {
    vector<string>::iterator i = m_PluginInfo.PortTips.end ();
    m_PluginInfo.PortTips.erase (i--);
    RemoveOutput();
    m_PluginInfo.NumOutputs--;
  }
  AsSigned(m_Chans) = n;
  // do the actual update
  UpdatePluginInfoWithHost ();
}

void SplitSwitchPlugin::Reset()
{
	ResetPorts();
	m_Triggered=false;
	AsSigned(m_Chans) = 2;
	m_GUIArgs.Switch = 1;
	m_GUIArgs.Echo = 1;
}

void SplitSwitchPlugin::Execute() {
  int n;
  int NumChans = m_PluginInfo.NumOutputs - 1;
  for (n=1; n<m_PluginInfo.NumOutputs; n++) GetOutputBuf(n)->Zero();
  if (InputExists (2)) {
    for (n=0; n<m_HostInfo->BUFSIZE; n++) {
      if (InputExists (0)) {
        m_GUIArgs.Auto = true;
        // Check the Switch Pos CV Value
        const float selection = fabs (GetInput (0, n));
        AsSigned(m_SwitchPos) = Spiral::Core::IsFinite (selection) && selection < NumChans + 1 ? lrint (selection) : 1;
      }
      else if (InputExists (1)) {
        m_GUIArgs.Auto = true;
        // Check the trigger CV value
        if (GetInput (1, n) < 0.01) {
          m_Triggered = false;
        }
        else {
          if (!m_Triggered) {
            m_Triggered = true;
            AsSigned(m_SwitchPos) = (AsSigned(m_SwitchPos)+1);
          }
        }
      }
      else {
        m_GUIArgs.Auto = false;
        AsSigned(m_SwitchPos)=(m_GUIArgs.Switch);
      }
      if (AsSigned(m_SwitchPos) > NumChans) AsSigned(m_SwitchPos) = 1;
      m_GUIArgs.Echo = AsSigned(m_SwitchPos);
      SetOutput (0, n, AsSigned(m_SwitchPos));
      SetOutput (AsSigned(m_SwitchPos), n, GetInput (2, n));
    }
  }
}

void SplitSwitchPlugin::Describe(Spiral::Core::Description &d) {
  d.Value(m_Version).Separator(" ").Value(AsSigned(m_Chans)).Separator(" ").Value(AsSigned(m_SwitchPos)).Separator(" ");
}

void SplitSwitchPlugin::Apply(Spiral::Core::Description::Reader &r) {
  int Version;
  r.Value(Version).Value(m_Chans).Value(m_SwitchPos);
  SetChans (AsSigned(m_Chans));
}
