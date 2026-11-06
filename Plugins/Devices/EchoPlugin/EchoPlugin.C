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
#include "EchoPlugin.h"
#include "SpiralIcon.xpm"

using namespace std;
using namespace Spiral::Core;

static const float MAX_DELAY=1.0f;

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
	return new EchoPlugin;
}

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DEVICE;
}

const char** SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}
}

///////////////////////////////////////////////////////

EchoPlugin::EchoPlugin() :
m_HeadPos (0),
m_Buf0 (0),
m_Buf1 (1)
{

	SetUnsigned(DeviceVersion, 2);

	m_Delay = FloatProperty(DefaultLinearFlags, 0.75, 0.0, 1.0, 0.001, 0.1);
	m_Feedback = FloatProperty(DefaultLinearFlags, 0.4, 00, 1.1, 0.01, 0.1);
	m_Bounce = ToggleProperty(Property::WriteOnly, false);
	RegisterSharedProperty(m_Delay, "Delay", "Delay");
	RegisterSharedProperty(m_Feedback, "Feedback", "Feedback");
	RegisterSharedProperty(m_Bounce, "Bounce", "Stereo Bounce");
        m_Version = 2;
        m_PluginInfo.Name = "Echo";
	m_PluginInfo.Width = 120;
	m_PluginInfo.Height = 140;
	m_PluginInfo.NumInputs = 3;
	m_PluginInfo.NumOutputs = 2;
	m_PluginInfo.PortTips.push_back ("Input");
	m_PluginInfo.PortTips.push_back ("Delay CV");
	m_PluginInfo.PortTips.push_back ("Feedback CV");
	m_PluginInfo.PortTips.push_back ("Left/Mono Out");
	m_PluginInfo.PortTips.push_back ("Right Out");
}

EchoPlugin::~EchoPlugin()
{
}

PluginInfo &EchoPlugin::Initialise (const HostInfo *Host)
{
	PluginInfo& Info = SpiralPlugin::Initialise (Host);
	m_Buffer[0].Allocate ((int)(m_HostInfo->SAMPLERATE * MAX_DELAY));
	m_Buffer[1].Allocate ((int)(m_HostInfo->SAMPLERATE * MAX_DELAY));
	return Info;
}

void EchoPlugin::Reset()
{
	ResetPorts();
	m_Buffer[0].Clear();
	m_Buffer[0].Allocate ((int)(m_HostInfo->SAMPLERATE * MAX_DELAY));

	m_Buffer[1].Clear();
	m_Buffer[1].Allocate ((int)(m_HostInfo->SAMPLERATE * MAX_DELAY));

	m_HeadPos = 0;
	m_Buf0 = 0;
	m_Buf1 = 1;
}

void EchoPlugin::Execute()
{
     float Delay;
     for (int n=0; n<m_HostInfo->BUFSIZE; n++) {
         Delay=( AsFloat(m_Delay)+GetInput (1,n)*0.5 ) * (float)m_HostInfo->SAMPLERATE-1;
         if (Delay >= MAX_DELAY*m_HostInfo->SAMPLERATE)
            Delay = (MAX_DELAY*m_HostInfo->SAMPLERATE)-1;
         if (Delay<0) Delay=0;
         if (m_HeadPos>Delay) m_HeadPos=0;
         if (AsBoolean(m_Bounce) && m_HeadPos==0) {
            int c=m_Buf0;
            m_Buf0=m_Buf1;
            m_Buf1=c;
         }
         // Write to the buffer
         m_Buffer[m_Buf0].Set (m_HeadPos, GetInput (0,n)+(m_Buffer[m_Buf0][m_HeadPos]*(AsFloat(m_Feedback)+GetInput (2,n))));
         if (!AsBoolean(m_Bounce)) m_Buffer[m_Buf1].Set (m_HeadPos, 0);
         // Read from the buffer
         SetOutput(0, n, m_Buffer[m_Buf0][m_HeadPos]);
         SetOutput(1, n, m_Buffer[m_Buf1][m_HeadPos]);
         m_HeadPos++;
     }

}

//void EchoPlugin::Randomise()
//{
//}

void EchoPlugin::Migrate(Spiral::Core::Description &d)
{
	if (d.Version() != 1)
		return;

	if (d.Values().size() == 3)
		d.Value(false);
	d.SetValue(0, 2);
}
