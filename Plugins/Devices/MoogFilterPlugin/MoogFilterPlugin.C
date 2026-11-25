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
#include <math.h>
#include "MoogFilterPlugin.h"
#include "SpiralIcon.xpm"

#define PI 3.141592654

using namespace std;
using namespace Spiral::Core;

static const int GRANULARITY = 10;

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
	return new MoogFilterPlugin;
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

MoogFilterPlugin::MoogFilterPlugin() :
fs(44100.0f),
fc(1000.0f),
f(0.0f),
p(0.0f),
q(0.0f),
b0(0.1f),
b1(0.0f),
b2(0.0f),
b3(0.0f),
b4(0.0f),
t1(0.0f),
t2(0.0f),
tpt_s1(0.0f),
tpt_s2(0.0f),
tpt_s3(0.0f),
tpt_s4(0.0f),
tpt_y4(0.0f)
{
	m_PluginInfo.Name="Moog Filter";
	m_PluginInfo.Width=120;
	m_PluginInfo.Height=130;
	m_PluginInfo.NumInputs=3;
	m_PluginInfo.NumOutputs=3;
	m_PluginInfo.PortTips.push_back("Input");	
	m_PluginInfo.PortTips.push_back("Cutoff CV");	
	m_PluginInfo.PortTips.push_back("Emphasis CV");	
	m_PluginInfo.PortTips.push_back("LowPass output");
	m_PluginInfo.PortTips.push_back("BandPass output");
	m_PluginInfo.PortTips.push_back("HighPass output");
	
	SetUnsigned(DeviceVersion, 1);

	Cutoff = FloatProperty(DefaultLinearFlags,0.5f, 0, 1, 0.0001, 0.001);
	Resonance = FloatProperty(DefaultLinearFlags,0.0f, 0, 1, 0.00001, 0.0001);
	Mode = UnsignedProperty(DefaultLinearFlags, 0, 0, 1, 1, 1);

	RegisterSharedProperty(Cutoff, "Cutoff", "Cutoff");
	RegisterSharedProperty(Resonance, "Resonance", "Resonance");
	RegisterSharedProperty(Mode, "Mode", "Mode");
}

MoogFilterPlugin::~MoogFilterPlugin()
{
}

PluginInfo &MoogFilterPlugin::Initialise(const HostInfo *Host)
{	
	PluginInfo& Info = SpiralPlugin::Initialise(Host);
	fs = m_HostInfo->SAMPLERATE;
	return Info;
}

void MoogFilterPlugin::Reset()
{
	ResetPorts();
	fs = m_HostInfo->SAMPLERATE;
	Clear();
}

void MoogFilterPlugin::Execute()
{
	if (AsUnsigned(Mode) == 1)
	{
		ExecuteTPT();
		return;
	}
	ExecuteLegacy();
}

void MoogFilterPlugin::ExecuteLegacy()
{
	float in, Q;
	
	// Early out?	
	// We don't want to early out if the resonance is set to
	// self oscillation, as this turns into an generator
	// in it's own right...
	/*if (Resonance<0.5 && !GetInput(2))
	{
		if (!m_Input[0] || m_Input[0]->IsEmpty())
		{
			cerr<<"moog early out"<<endl;
		
			m_Output[0]->Zero();
			m_Output[1]->Zero();	
			m_Output[2]->Zero();				
			return;
		}
	}*/
	
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		if (n%GRANULARITY==0)
		{
			fc = (AsFloat(Cutoff)+GetInput(1,n))*0.25;
			if (fc<0) fc=0;
			
			q = 1.0f - fc;
			p = fc + 0.8f * fc * q;
			f = p + p - 1.0f;
			Q = ((AsFloat(Resonance)+GetInput(2,n))*6)-3.0f;
			q = Q + (1.0f + 0.5f * q * (1.0f - q + 5.6f * q * q));
		}

		in = GetInput(0,n);
		
		if (in == 0)
		{
			Clear();
			SetOutput(0,n,0);	 
	  		SetOutput(1,n,0);
			SetOutput(2,n,0);	
		} else {				
			in -= q * b4;
			
			if (in>1) in=1;
			if (in<-1) in=-1;
									
			t1 = b1; b1 = (in + b0) * p - b1 * f;
			t2 = b2; b2 = (b1 + t1) * p - b2 * f;
			t1 = b3; b3 = (b2 + t2) * p - b3 * f;		
     			     b4 = (b3 + t1) * p - b4 * f;	
			b4 = b4 - b4 * b4 * b4 * 0.166667f;
			
			b0 = in;	
			 
			SetOutput(0,n,b4);	 
			SetOutput(1,n,(in-b4));
			SetOutput(2,n,3.0f * (b3 - b4));	
		}
	}			
}

void MoogFilterPlugin::ExecuteTPT()
{
	// Zavalishin, The Art of VA Filter Design, ch. 5:
	// resolved zero-delay feedback around four TPT 1-poles,
	// with tanh in the feedback path (implicit, Newton).
	float G = 0.0f, Gall = 0.0f, k = 0.0f, tptg = 0.0f;
	const float ny = 0.49f * fs;
	const float minHz = 20.0f;
	const float ratio = (ny > minHz && fs > 0.0f) ? (ny / minHz) : 1.0f;

	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		if (n%GRANULARITY==0)
		{
			float cut = AsFloat(Cutoff)+GetInput(1,n);
			if (cut<0.0f) cut=0.0f;
			if (cut>1.0f) cut=1.0f;
			float hz = minHz * powf(ratio, cut);
			if (fs > 0.0f)
				tptg = tanf((float)PI * hz / fs);
			else
				tptg = 0.0f;
			if (tptg<0.0f) tptg=0.0f;
			G = tptg / (1.0f + tptg);
			Gall = G*G*G*G;
			float res = AsFloat(Resonance)+GetInput(2,n);
			if (res<0.0f) res=0.0f;
			if (res>1.0f) res=1.0f;
			k = res * 4.05f;
		}

		float x = GetInput(0,n);
		float inv = 1.0f / (1.0f + tptg);
		float S1 = tpt_s1 * inv;
		float S2 = tpt_s2 * inv;
		float S3 = tpt_s3 * inv;
		float S4 = tpt_s4 * inv;
		float S = ((S1*G + S2)*G + S3)*G + S4;

		float y4 = tpt_y4;
		float denom = 1.0f + k*Gall;
		float lin = (denom != 0.0f) ? (Gall*x + S) / denom : S;
		if (y4 == 0.0f) y4 = lin;
		for (int i=0; i<4; i++)
		{
			float th = tanhf(y4);
			float F = y4 - Gall*(x - k*th) - S;
			float Fp = 1.0f + Gall*k*(1.0f - th*th);
			if (Fp != 0.0f) y4 -= F/Fp;
		}

		float x0 = x - k * tanhf(y4);

		float v1 = (x0 - tpt_s1) * G;
		float y1 = v1 + tpt_s1;
		tpt_s1 = y1 + v1;

		float v2 = (y1 - tpt_s2) * G;
		float y2 = v2 + tpt_s2;
		tpt_s2 = y2 + v2;

		float v3 = (y2 - tpt_s3) * G;
		float y3 = v3 + tpt_s3;
		tpt_s3 = y3 + v3;

		float v4 = (y3 - tpt_s4) * G;
		float y4o = v4 + tpt_s4;
		tpt_s4 = y4o + v4;
		tpt_y4 = y4o;

		if (!isfinite(tpt_s1) || !isfinite(tpt_s2) ||
		    !isfinite(tpt_s3) || !isfinite(tpt_s4) ||
		    !isfinite(y4o) || !isfinite(y3))
		{
			tpt_s1=tpt_s2=tpt_s3=tpt_s4=tpt_y4=0.0f;
			y4o=0.0f;
			y3=0.0f;
		}
		else
		{
			if (tpt_s1!=0.0f && fabsf(tpt_s1)<1.0e-20f) tpt_s1=0.0f;
			if (tpt_s2!=0.0f && fabsf(tpt_s2)<1.0e-20f) tpt_s2=0.0f;
			if (tpt_s3!=0.0f && fabsf(tpt_s3)<1.0e-20f) tpt_s3=0.0f;
			if (tpt_s4!=0.0f && fabsf(tpt_s4)<1.0e-20f) tpt_s4=0.0f;
		}

		SetOutput(0,n,y4o);
		SetOutput(1,n,(x-y4o));
		SetOutput(2,n,3.0f * (y3 - y4o));
	}
}

void MoogFilterPlugin::Clear()
{
	b0 = 0.0f;
	b1 = 0.0f;
	b2 = 0.0f;
	b3 = 0.0f;
	b4 = 0.0f;
	t1 = 0.0f;
	t2 = 0.0f;
	tpt_s1 = 0.0f;
	tpt_s2 = 0.0f;
	tpt_s3 = 0.0f;
	tpt_s4 = 0.0f;
	tpt_y4 = 0.0f;
}

void MoogFilterPlugin::Randomise()
{
}
	
