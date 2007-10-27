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
#include <stdio.h>
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
m_SwitchPos (0),
m_Triggered (false)
{
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
	// Number of channels <- GUI
	Chans = UnsignedProperty(DefaultLinearFlags | Property::ForceUpdate, 2, 2, 100, 1, 1);
	RegisterSharedProperty(Chans, "Chans", "Chans");

	// Switch Position <- GUI
	Switch = UnsignedProperty(DefaultLinearFlags, 1, 1, 100, 1, 1);
	RegisterSharedProperty(Switch, "Switch", "Switch");

	// Switch Position -> GUI
	Echo = UnsignedProperty(Property::ReadOnly , 1, 0, 0, 0, 0);
	RegisterSharedProperty(Echo, "Echo", "Echo");

	// Auto Mode (Switch position is controlled by CV or Clock = True, Switch Position controlled by GUI = False
	Auto = ToggleProperty(Property::ReadOnly, false);
	RegisterSharedProperty(Auto, "Auto", "Auto");

	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 1);
}

SplitSwitchPlugin::~SplitSwitchPlugin () {
}

PluginInfo &SplitSwitchPlugin::Initialise (const HostInfo *Host) {
  return SpiralPlugin::Initialise (Host);
}



void SplitSwitchPlugin::ExecuteCommands () {
  if (m_AudioCH->IsCommandWaiting ()) {
    switch (m_AudioCH->GetCommand()) {
      case (SETCHANS)  : SetChans (AsUnsigned(Chans));
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
  // do the actual update
  UpdatePluginInfoWithHost ();
}

void SplitSwitchPlugin::Reset()
{
	ResetPorts();
	m_Triggered=false;
	AsUnsigned(Chans) = 2;
	AsUnsigned(Switch) = 1;
	AsUnsigned(Echo) = 1;
}

void SplitSwitchPlugin::Execute() {
  int n;
  int NumChans = m_PluginInfo.NumOutputs - 1;
  for (n=1; n<m_PluginInfo.NumOutputs; n++) GetOutputBuf(n)->Zero();
  if (InputExists (2)) {
    for (n=0; n<m_HostInfo->BUFSIZE; n++) {
      if (InputExists (0)) {
        AsBoolean(Auto) = true;
        // Check the Switch Pos CV Value
        m_SwitchPos = int (GetInput (0, n));
      }
      else if (InputExists (1)) {
        AsBoolean(Auto) = true;
        // Check the trigger CV value
        if (GetInput (1, n) < 0.01) {
          m_Triggered = false;
        }
        else {
          if (!m_Triggered) {
            m_Triggered = true;
            m_SwitchPos = (m_SwitchPos+1);
          }
        }
      }
      else {
        AsBoolean(Auto) = false;
        m_SwitchPos=(AsUnsigned(Switch));
      }
      if (m_SwitchPos > NumChans) m_SwitchPos = 1;
      AsUnsigned(Echo) = m_SwitchPos;
      SetOutput (0, n, m_SwitchPos);
      SetOutput (m_SwitchPos, n, GetInput (2, n));
    }
  }
}

void SplitSwitchPlugin::Describe(Spiral::Core::Description &d) {
  d.Value(m_Version).Separator(" ").Value(m_PluginInfo.NumOutputs - 1).Separator(" ").Value(m_SwitchPos).Separator(" ");
}

void SplitSwitchPlugin::Apply(Spiral::Core::Description::Reader &r) {
  int Version, Chans, SwitchPos;
  r.Value(Version).Value(Chans).Value(SwitchPos);
  SetChans (Chans);
  m_SwitchPos = SwitchPos;
}
