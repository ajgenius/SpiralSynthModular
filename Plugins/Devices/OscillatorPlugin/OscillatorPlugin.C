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
#include "OscillatorPlugin.h"
#include <limits.h>
#include "SpiralIcon.xpm"
#include <stdlib.h>

using namespace std;
using namespace Spiral::Core;

static const int IN_FREQ  = 0;
static const int IN_PW    = 1;
static const int IN_SHLEN = 2;

static const int OUT_MAIN = 0;

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
	return new OscillatorPlugin;
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
	return 0x0004;
}

string SpiralPlugin_GetName()
{
	return "Oscillator";
}

string SpiralPlugin_GetGroupName()
{
	return "Oscillators";
}
}

///////////////////////////////////////////////////////

OscillatorPlugin::OscillatorPlugin() :
m_Noisev(0),
m_FreqModBuf(NULL),
m_PulseWidthModBuf(NULL),
m_SHModBuf(NULL)
{
	const NamedPropertyValue WaveTypes[] =
	{
		{"Square", DefaultUnsigned(SQUARE)},
		{"Saw", DefaultUnsigned(SQUARE)},
		{"Noise", DefaultUnsigned(NOISE)}
	};

	m_Type = SetProperty(Property::WriteOnly, 0, NamedPropertySet(WaveTypes, sizeof(WaveTypes)/sizeof(WaveTypes[0])));
	m_Octave = UnsignedProperty(DefaultLinearFlags,0,0,6,1,1);
	m_FineFreq = FloatProperty(DefaultLinearFlags,1, 0, 1.414, 0.000001, 0.0001);
	m_PulseWidth = FloatProperty(DefaultLinearFlags, 0.5f, 0, 1, 0.01, 0.1);
	m_ModAmount = FloatProperty(DefaultLinearFlags, 1.0f, 0, 2.0f, 0.001, 0.01);
	m_SHLen = FloatProperty(DefaultLinearFlags, 0.1f, 0, 0.2f, 0.001, 0.01);

	RegisterSharedProperty(m_Type, "Wave Type", "Wave Type");
	RegisterSharedProperty(m_Octave, "Octave", "Octave");
	RegisterSharedProperty(m_FineFreq, "Fine Tune", "Fine Tune Frequency");
	RegisterSharedProperty(m_PulseWidth, "Pulse Width", "Pulse Width");
	RegisterSharedProperty(m_ModAmount, "ModAmount", "ModAmount");
	RegisterSharedProperty(m_SHLen, "Sample & Hold Length", "Sample & Hold Length");

	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 1);

	m_CyclePos=0;
	m_Note=0;
	m_LastFreq=0;

	m_PluginInfo.Name="Oscillator";
	m_PluginInfo.Width=210;
	m_PluginInfo.Height=140;
	m_PluginInfo.NumInputs=3;
	m_PluginInfo.NumOutputs=1;
	m_PluginInfo.PortTips.push_back("Frequency CV");
	m_PluginInfo.PortTips.push_back("PulseWidth CV");
	m_PluginInfo.PortTips.push_back("Sample & Hold length CV");
	m_PluginInfo.PortTips.push_back("Output");

}

OscillatorPlugin::~OscillatorPlugin()
{
}

PluginInfo &OscillatorPlugin::Initialise(const HostInfo *Host)
{
	return SpiralPlugin::Initialise(Host);
}



void OscillatorPlugin::Reset()
{
	ResetPorts();

	m_CyclePos=0;
	m_Note=0;
	m_LastFreq=0;
}

void OscillatorPlugin::Execute()
{
	short noisev=0;
	float Freq=0;
	float CycleLen=0;
	int samplelen, PW;

	switch (AsUnsigned(m_Type))
	{
	case SQUARE:
		for (int n=0; n<m_HostInfo->BUFSIZE; n++)
		{
			if (InputExists(0)) Freq=GetInputPitch(0,n);
			else Freq=110;
			Freq*=AsFloat(m_FineFreq);
			if (AsUnsigned(m_Octave)>0) Freq*=1<<(AsUnsigned(m_Octave));
			if (AsUnsigned(m_Octave)<0) Freq/=1<<(-AsUnsigned(m_Octave));
			CycleLen = m_HostInfo->SAMPLERATE/Freq;
			PW = (int)((AsFloat(m_PulseWidth)+GetInput(IN_PW,n)*AsFloat(m_ModAmount)) * CycleLen);

			// calculate square wave pattern
			m_CyclePos++;
			if (m_CyclePos>CycleLen) m_CyclePos=0;

			if (m_CyclePos<PW) SetOutput(OUT_MAIN,n,1);
			else SetOutput(OUT_MAIN,n,-1);
		}
		break;

	case SAW:
		for (int n=0; n<m_HostInfo->BUFSIZE; n++)
		{
			if (InputExists(0)) Freq=GetInputPitch(0,n);
			else Freq=110;
			Freq*=AsFloat(m_FineFreq);
			if (AsUnsigned(m_Octave)>0) Freq*=1<<(AsUnsigned(m_Octave));
			if (AsUnsigned(m_Octave)<0) Freq/=1<<(-AsUnsigned(m_Octave));
			CycleLen = m_HostInfo->SAMPLERATE/Freq;
			PW = (int)((AsFloat(m_PulseWidth)+GetInput(IN_PW,n)*AsFloat(m_ModAmount)) * CycleLen);

			// get normailise position between cycle
			m_CyclePos++;
			if (m_CyclePos>CycleLen) m_CyclePos=0;

			if (m_CyclePos<PW)
			{
				// before pw -1->1
				SetOutput(OUT_MAIN,n,Linear(0,PW,m_CyclePos,-1,1));
			}
			else
			{
				// after pw 1->-1
				SetOutput(OUT_MAIN,n,Linear(PW,CycleLen,m_CyclePos,1,-1));
			}
		}
		break;

	case NOISE:
		for (int n=0; n<m_HostInfo->BUFSIZE; n++)
		{
			m_CyclePos++;

			//modulate the sample & hold length
			samplelen = (int)((AsFloat(m_SHLen)+GetInput(IN_SHLEN,n)*AsFloat(m_ModAmount))*m_HostInfo->SAMPLERATE);

			// do sample & hold on the noise
			if (m_CyclePos>samplelen)
			{
				m_Noisev=(short)((rand()%SHRT_MAX*2)-SHRT_MAX);
				m_CyclePos=0;
			}
			SetOutput(OUT_MAIN,n,m_Noisev/(float)SHRT_MAX);
		}
		break;

	case NONE: break;
	}
}

void OscillatorPlugin::Describe(Spiral::Core::Description &d)
{
	d.Value(m_Version).Separator(" ");
	::Describe(d, *this);
}

void OscillatorPlugin::Apply(Spiral::Core::Description::Reader &r)
{
	int version;
	r.Value(version);
	::Apply(r, *this);
}

Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, OscillatorPlugin &o)
{
	float dummy=0;
	r.Value((int&)AsUnsigned(o.m_Type)).Value(AsUnsigned(o.m_Octave)).Value(AsFloat(o.m_FineFreq)).Value(AsFloat(o.m_PulseWidth)).Value(dummy)
	.Value(AsFloat(o.m_SHLen)).Value(AsFloat(o.m_ModAmount));
	return r;
}

Spiral::Core::Description &Describe(Spiral::Core::Description &d, OscillatorPlugin &o)
{
	float dummy=0;
	d.Value((int)AsUnsigned(o.m_Type)).Separator(" ").Value(AsUnsigned(o.m_Octave)).Separator(" ").Value(AsFloat(o.m_FineFreq)).Separator(" ").Value(AsFloat(o.m_PulseWidth)).Separator(" ")
	.Value(dummy).Separator(" ").Value(AsFloat(o.m_SHLen)).Separator(" ").Value(AsFloat(o.m_ModAmount)).Separator(" ");
	return d;
}
