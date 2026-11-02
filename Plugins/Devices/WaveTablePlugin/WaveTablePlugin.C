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
#include "WaveTablePlugin.h"
#include <cmath>
#include "Finite.h"
#include "SpiralIcon.xpm"

using namespace std;
using namespace Spiral::Core;

static const int IN_FREQ  = 0;
static const int IN_PW    = 1;
static const int IN_SHLEN = 2;

static const int OUT_MAIN = 0;

#include <config.h>

extern "C"
{
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
	return new WaveTablePlugin;
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
	return 0x0017;
}

string SpiralPlugin_GetName()
{
	return "WaveTable";
}

string SpiralPlugin_GetGroupName()
{
	return "Oscillators";
}
}

///////////////////////////////////////////////////////

WaveTablePlugin::WaveTablePlugin() :
m_TableLength(DEFAULT_TABLE_LEN)
{
	const NamedPropertyValue WaveTypes[] =
	{
		{"Sine", DefaultUnsigned(SINE)},
		{"Square", DefaultUnsigned(SQUARE)},
		{"Saw", DefaultUnsigned(SAW)},
		{"Reverse Saw", DefaultUnsigned(REVSAW)},
		{"Triangle", DefaultUnsigned(TRIANGLE)},
		{"Pulse 1", DefaultUnsigned(PULSE1)},
		{"Pulse 2", DefaultUnsigned(PULSE2)},
		{"Inverse Sine", DefaultUnsigned(INVSINE)}
	};

	m_Type = SetProperty(Property::WriteOnly, 0, NamedPropertySet(WaveTypes, sizeof(WaveTypes)/sizeof(WaveTypes[0])));
	m_Octave = SignedProperty(DefaultLinearFlags,0,-3,3,1,1);
	m_FineFreq = FloatProperty(DefaultLinearFlags,1, 0, 1.414, 0.000001, 0.0001);
	m_ModAmount = FloatProperty(DefaultLinearFlags, 1.0f, 0, 2.0f, 0.001, 0.01);

	RegisterSharedProperty(m_Type, "Wave Type", "Wave Type");
	RegisterSharedProperty(m_Octave, "Octave", "Octave");
	RegisterSharedProperty(m_FineFreq, "FineFreq", "FineFreq");
	RegisterSharedProperty(m_ModAmount, "ModAmount", "ModAmount");

	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 1);
	m_CyclePos=0;	
	m_Note=0;
	
	m_PluginInfo.Name="WaveTable";
	m_PluginInfo.Width=210;
	m_PluginInfo.Height=140;
	m_PluginInfo.NumInputs=1;
	m_PluginInfo.NumOutputs=1;
	m_PluginInfo.PortTips.push_back("Frequency CV");
	m_PluginInfo.PortTips.push_back("Output");
	
}

WaveTablePlugin::~WaveTablePlugin()
{
}

PluginInfo &WaveTablePlugin::Initialise(const HostInfo *Host)
{	
	PluginInfo& Info= SpiralPlugin::Initialise(Host);
	
	for (int n=0; n<NUM_TABLES; n++)
	{
		m_Table[n].Allocate(m_TableLength);
	}
	
	WriteWaves();
		
	return Info;
}



void WaveTablePlugin::WriteWaves()
{
	float RadCycle = (M_PI/180)*360;
	float Pos=0;

	for (int n=0; n<m_TableLength; n++)
	{
		if (n==0) Pos=0;
		else Pos=(n/(float)m_TableLength)*RadCycle;
		m_Table[SINE].Set(n,sin(Pos));		
	}

	for (int n=0; n<m_TableLength; n++)
	{
		if (n<m_TableLength/2) m_Table[SQUARE].Set(n,1.0f);
		else m_Table[SQUARE].Set(n,-1);				
	}
	
	for (int n=0; n<m_TableLength; n++)
	{
		m_Table[REVSAW].Set(n,((n/(float)m_TableLength)*2.0f)-1.0f);
	}
	
	for (int n=0; n<m_TableLength; n++)
	{
		m_Table[SAW].Set(n,1-(n/(float)m_TableLength)*2.0f);
	}

	float HalfTab=m_TableLength/2;
	float v=0;
	for (int n=0; n<m_TableLength; n++)
	{
		if (n<HalfTab) v=1-(n/HalfTab)*2.0f;
		else v=(((n-HalfTab)/HalfTab)*2.0f)-1.0f;
		v*=0.99;
		m_Table[TRIANGLE].Set(n,v);		
	}

	for (int n=0; n<m_TableLength; n++)
	{
		if (n<m_TableLength/1.2) m_Table[PULSE1].Set(n,1);
		else m_Table[PULSE1].Set(n,-1);				
	}
	
	for (int n=0; n<m_TableLength; n++)
	{
		if (n<m_TableLength/1.5) m_Table[PULSE2].Set(n,1);
		else m_Table[PULSE2].Set(n,-1);				
	}

	Pos=0;
	for (int n=0; n<m_TableLength; n++)
	{
		if (n==0) Pos=0;
		else Pos=(n/(float)m_TableLength)*RadCycle;
		if (sin(Pos)==0) m_Table[INVSINE].Set(n,0);
		else m_Table[INVSINE].Set(n,(1.0f/sin(Pos))/10.0f);				
	}
}

void WaveTablePlugin::Reset()
{
	ResetPorts();

	for (int n=0; n<NUM_TABLES; n++)
	{
		m_Table[n].Clear();
		m_Table[n].Allocate(m_TableLength);
	}
	
	WriteWaves();
	
	m_CyclePos=0;	
	m_Note=0;
}

void WaveTablePlugin::Execute()
{
 	float Freq=0;
	float Incr;
	
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{	
		SetOutput(OUT_MAIN,n,0);
		
		if (InputExists(0)) 
		{
			Freq=GetInputPitch(0,n);
			Freq*=AsFloat(m_ModAmount);
		}
		else 
		{	
			Freq=110;
		}

		Freq*=AsFloat(m_FineFreq);
		if (AsSigned(m_Octave)>0) Freq*=1<<(AsSigned(m_Octave));
		if (AsSigned(m_Octave)<0) Freq/=1<<(-AsSigned(m_Octave));
		
		Incr = Freq*(m_TableLength/(float)m_HostInfo->SAMPLERATE);
		m_CyclePos+=Incr;
		
		if (!Spiral::Core::IsFinite(m_CyclePos)) m_CyclePos=0;
		if (m_CyclePos>=m_TableLength) m_CyclePos=fmodf(m_CyclePos,m_TableLength);
		
		if (m_CyclePos<0 || m_CyclePos>=m_TableLength) m_CyclePos=0;
		
		SetOutput(OUT_MAIN,n,m_Table[static_cast<unsigned char>(AsUnsigned(m_Type))][m_CyclePos]);
	}
}

void WaveTablePlugin::Describe(Spiral::Core::Description &d)
{
	d.Value(m_Version).Separator(" ");
	::Describe(d, *this);
}

void WaveTablePlugin::Apply(Spiral::Core::Description::Reader &r)
{
	int version;
	r.Value(version);
	::Apply(r, *this);
}

Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, WaveTablePlugin &o)
{
	r.Value((int&)AsUnsigned(o.m_Type)).Value(AsSigned(o.m_Octave)).Value(AsFloat(o.m_FineFreq)).Value(AsFloat(o.m_ModAmount));
	return r;
}

Spiral::Core::Description &Describe(Spiral::Core::Description &d, WaveTablePlugin &o)
{
	d.Value((int)AsUnsigned(o.m_Type)).Separator(" ").Value(AsSigned(o.m_Octave)).Separator(" ").Value(AsFloat(o.m_FineFreq)).Separator(" ").Value(AsFloat(o.m_ModAmount)).Separator(" ");
	return d;
}
