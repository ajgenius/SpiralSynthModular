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
#include <limits>
#include <cmath>
#include "Finite.h"
#include <limits.h>
#include "SpiralIcon.xpm"
#include <stdlib.h>
#include <algorithm>

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
		{"None", DefaultUnsigned(NONE)},
		{"Square", DefaultUnsigned(SQUARE)},
		{"Saw", DefaultUnsigned(SAW)},
		{"Noise", DefaultUnsigned(NOISE)}
	};

	m_Type = SetProperty(Property::WriteOnly, SQUARE, NamedPropertySet(WaveTypes, sizeof(WaveTypes)/sizeof(WaveTypes[0])));
	m_Octave = SignedProperty(DefaultLinearFlags,OCTAVE_ORIGIN,0,10,1,1);
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

	SetUnsigned(DeviceVersion, 2);

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

static int BoundedCycleOffset(float value)
{
	const int limit = std::numeric_limits<int>::max() / 2;
	if (!Spiral::Core::IsFinite(value)) return 0;
	if (value >= limit) return limit;
	if (value <= -limit) return -limit;
	return static_cast<int>(value);
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
			Freq=ldexpf(Freq,OctaveShift());
			CycleLen = m_HostInfo->SAMPLERATE/Freq;
			PW = BoundedCycleOffset((AsFloat(m_PulseWidth)+GetInput(IN_PW,n)*AsFloat(m_ModAmount)) * CycleLen);

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
			Freq=ldexpf(Freq,OctaveShift());
			CycleLen = m_HostInfo->SAMPLERATE/Freq;
			PW = BoundedCycleOffset((AsFloat(m_PulseWidth)+GetInput(IN_PW,n)*AsFloat(m_ModAmount)) * CycleLen);

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
				SetOutput(OUT_MAIN,n,PW==CycleLen ? 1 : Linear(PW,CycleLen,m_CyclePos,1,-1));
			}
		}
		break;

	case NOISE:
		for (int n=0; n<m_HostInfo->BUFSIZE; n++)
		{
			m_CyclePos++;

			//modulate the sample & hold length
			samplelen = BoundedCycleOffset((AsFloat(m_SHLen)+GetInput(IN_SHLEN,n)*AsFloat(m_ModAmount))*m_HostInfo->SAMPLERATE);

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
	d.Value(DeviceVersion).Separator(" ");
	::Describe(d, *this);
}

void OscillatorPlugin::Apply(Spiral::Core::Description::Reader &r)
{
	int version;
	r.Value(version);
	::Apply(r, *this);
}

void OscillatorPlugin::Migrate(Spiral::Core::Description &d)
{
	if (d.Version() < 0 || d.Version() >= 2)
		return;

	int type = d.Field(1, "Wave Type");
	if (type >= 0)
		d.SetValue(type, std::min(std::max(atoi(d.Values()[type].c_str()), (int)NONE), (int)NOISE));

	int octave = d.Field(2, "Octave");
	if (octave >= 0)
		d.SetValue(octave, std::min(std::max(atoi(d.Values()[octave].c_str()) + OCTAVE_ORIGIN, 0), 10));

	d.SetValue(0, 2);
}

Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, OscillatorPlugin &o)
{
	float dummy=0;
	r.Value(o.m_Type).Value(o.m_Octave).Value(o.m_FineFreq).Value(o.m_PulseWidth);
	if (!r.Named()) r.Value(dummy);

	r.Value(o.m_SHLen).Value(o.m_ModAmount);
	return r;
}

Spiral::Core::Description &Describe(Spiral::Core::Description &d, OscillatorPlugin &o)
{
	float dummy=0;
	d.Value(o.m_Type).Separator(" ").Value(o.m_Octave).Separator(" ").Value(o.m_FineFreq).Separator(" ").Value(o.m_PulseWidth).Separator(" ")
	.Value(dummy).Separator(" ").Value(o.m_SHLen).Separator(" ").Value(o.m_ModAmount).Separator(" ");
	return d;
}
