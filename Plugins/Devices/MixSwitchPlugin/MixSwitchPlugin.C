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
#include "MixSwitchPlugin.h"
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


   SpiralPlugin* SpiralPlugin_CreateInstance () { return new MixSwitchPlugin; }

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DEVICE;
}


   const char** SpiralPlugin_GetIcon () { return SpiralIcon_xpm; }

   int SpiralPlugin_GetID () { return 126; }

   string SpiralPlugin_GetName()
   {
       return "MixSwitch";
   }

   string SpiralPlugin_GetGroupName() { return "Maths/Logic"; }
}

///////////////////////////////////////////////////////

MixSwitchPlugin::MixSwitchPlugin () :
m_Position (1),
m_Triggered (false)
{
	m_SwitchPos = SignedProperty(DefaultLinearFlags, 1, 0, 100, 1, 1);
	RegisterSharedProperty(m_SwitchPos, "Switch Position", "Selected input");
  m_Chans = SignedProperty(DefaultLinearFlags, 2, 1, 100, 1, 1);
  RegisterSharedProperty(m_Chans, "Channels", "Number of channels");
  m_Echo = UnsignedProperty(Property::ReadOnly, 1, 0, 0, 0, 0);
  RegisterSharedProperty(m_Echo, "Echo", "The input in effect");
  m_Auto = ToggleProperty(Property::ReadOnly, false);
  RegisterSharedProperty(m_Auto, "Auto", "Switched by the CV or the clock");
  m_PluginInfo.Name = "MixSwitch";
  m_PluginInfo.Width = 80;
  m_PluginInfo.Height = 80;
  CreatePorts ();

	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 1);
}

MixSwitchPlugin::~MixSwitchPlugin () {
}

PluginInfo &MixSwitchPlugin::Initialise (const HostInfo *Host) {
  return SpiralPlugin::Initialise (Host);
}



void MixSwitchPlugin::ExecuteCommands () {
  if (AsSigned(m_Chans) != GetChans ()) SetChans (AsSigned(m_Chans));
}
void MixSwitchPlugin::CreatePorts (int n, bool AddPorts) {
  int c;
  m_PluginInfo.NumInputs = 2 + n;
  m_PluginInfo.PortTips.push_back ("CV");
  m_PluginInfo.PortTips.push_back ("Clock");
  char t[256];
  for (c=1; c<=n; c++) {
    sprintf (t, "In %d", c);
    m_PluginInfo.PortTips.push_back (t);
  }
  m_PluginInfo.NumOutputs = 2;
  m_PluginInfo.PortTips.push_back ("CV");
  m_PluginInfo.PortTips.push_back ("Out");
  if (AddPorts) {
    for (c=0; c<m_PluginInfo.NumInputs; c++) AddInput();
    for (c=0; c<m_PluginInfo.NumOutputs; c++) AddOutput();
  }
}

void MixSwitchPlugin::SetChans (int n) {
  // once to clear the connections with the current info
  // do we need this????
  UpdatePluginInfoWithHost();
  // Things can get a bit confused deleting and adding inputs
  // so we just chuck away all the ports...
  RemoveAllInputs ();
  RemoveAllOutputs ();
  m_PluginInfo.NumInputs = 0;
  m_PluginInfo.NumOutputs = 0;
  m_PluginInfo.PortTips.clear ();
  // ... and then create some new ones
  CreatePorts (n, true);
  AsSigned(m_Chans) = n;
  // do the actual update
  UpdatePluginInfoWithHost ();
}


void MixSwitchPlugin::Reset()
{
	ResetPorts();
	m_Position = 1;
	m_Triggered = false;
}

void MixSwitchPlugin::Execute() {
  float f;
  int p;
  int NumChans = m_PluginInfo.NumInputs - 2;
  for (int n=0; n<m_HostInfo->BUFSIZE; n++) {
    if (InputExists (0)) {
      AsBoolean(m_Auto) = true;
      // Check the Switch Pos CV Value
      const float selection = fabs (GetInput (0, n));
      m_Position = Spiral::Core::IsFinite (selection) && selection < NumChans + 1 ? lrint (selection) : 1;
    }
    else if (InputExists (1)) {
      AsBoolean(m_Auto) = true;
      // Check the trigger CV value
      if (GetInput (1, n) < 0.01) {
        m_Triggered = false;
      }
      else {
        if (!m_Triggered) {
          m_Triggered = true;
          m_Position = m_Position + 1;
        }
      }
    }
    else {
      AsBoolean(m_Auto) = false;
      m_Position = AsSigned(m_SwitchPos);
    }
    if (m_Position > NumChans) m_Position = 1;
    AsUnsigned(m_Echo) = m_Position;
    SetOutput (0, n, m_Position);
    p = m_Position + 1; // SwitchPos 1 = Port 2, and so on
    if (InputExists (p)) f=GetInput (p, n);
    else f=0.0;
    SetOutput (1, n, f);
  }
}

void MixSwitchPlugin::Describe(Spiral::Core::Description &d) {
  d.Value(m_Version).Separator(" ").Value(m_Chans).Separator(" ").Value(m_SwitchPos).Separator(" ");
}

void MixSwitchPlugin::Apply(Spiral::Core::Description::Reader &r) {
  int Version;
  r.Value(Version).Value(m_Chans).Value(m_SwitchPos);
  m_Chans.Clamp();
  SetChans (AsSigned(m_Chans));
}
