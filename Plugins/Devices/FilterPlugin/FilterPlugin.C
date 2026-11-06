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
#include "FilterPlugin.h"
#include "SpiralIcon.xpm"

using namespace std;
using namespace Spiral::Core;

static const float MAX_RES = 10;
static const float MIN_RES = 1;
static const float MAX_CUTOFF = 3000;
static const float MIN_CUTOFF = 500;
static const int   FILTERGRAN = 50;

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
	return new FilterPlugin;
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

FilterPlugin::FilterPlugin() :
fs(0),     			// Sampling frequency
m_LastFC(0),
m_LastQ(1),
k(1.0)               // Set overall filter gain
{
	m_PluginInfo.Name="Filter";
	m_PluginInfo.Width=120;
	m_PluginInfo.Height=110;
	m_PluginInfo.NumInputs=3;
	m_PluginInfo.NumOutputs=1;
	m_PluginInfo.PortTips.push_back("Input");	
	m_PluginInfo.PortTips.push_back("Cutoff CV");	
	m_PluginInfo.PortTips.push_back("Emphasis CV");	
	m_PluginInfo.PortTips.push_back("Output");
	
	SetUnsigned(DeviceVersion, 2);

	fc = FloatProperty(DefaultLinearFlags, 100, 0, 100, 0.01,  1);
	Q = FloatProperty(DefaultLinearFlags, 1, 0, 10, 0.01, 0.1);

	RegisterSharedProperty(fc, "Cutoff", "Cutoff");
	RegisterSharedProperty(Q, "Resonance", "Resonance");

	m_RevCutoffMod = ToggleProperty(Property::WriteOnly, false);
	m_RevResonanceMod = ToggleProperty(Property::WriteOnly, false);
	RegisterSharedProperty(m_RevCutoffMod, "RevC", "Reverse cutoff CV");
	RegisterSharedProperty(m_RevResonanceMod, "RevR", "Reverse resonance CV");
	
}

FilterPlugin::~FilterPlugin()
{
}

PluginInfo &FilterPlugin::Initialise(const HostInfo *Host)
{	
	PluginInfo& Info = SpiralPlugin::Initialise(Host);
	
	fs=m_HostInfo->SAMPLERATE;
	
	SetupCoeffs();
	
    iir.length = FILTER_SECTIONS;         	
    iir.coef = (float *) calloc(4 * iir.length + 1 , sizeof(float)); 
	
	if (!iir.coef) 
    { 
         printf("Unable to allocate coef array, exiting\n"); 
         exit(1); 
    } 		
	
	return Info;
}

void FilterPlugin::Reset()
{
	ResetPorts();

	fs=m_HostInfo->SAMPLERATE;
	SetupCoeffs();
}

void FilterPlugin::Execute()
{
	float Cutoff;
	float Resonance;
	float out, in;

	if (AsFloat(fc)<0) return;
   
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{

		//reset memory if disconnected, and skip out (prevents CPU spike)
		if (! InputExists(0)) 
		{	
			out = 0;	 
		}
		else
		{
			in = GetInput(0,n);
			
			// work around denormal calculation CPU spikes where in --> 0
			if ((in >= 0) && (in < 0.000000001))
				in += 0.000000001;
			else
				if ((in <= 0) && (in > -0.000000001))
					in -= 0.000000001;

			coef = iir.coef + 1;     /* Skip k, or gain */ 
			k=0.25;
	
			Cutoff = AsFloat(fc) + (AsBoolean(m_RevCutoffMod) ? -GetInput(1,n) : GetInput(1,n)) * 1000;
			Resonance = AsFloat(Q) + (AsBoolean(m_RevResonanceMod) ? -GetInput(2,n) : GetInput(2,n));
	 	
			Cutoff/=2;
	 
			if (Resonance>MAX_RES) Resonance=MAX_RES; 
			if (Cutoff>MAX_CUTOFF) Cutoff=MAX_CUTOFF; 
			if (Resonance<MIN_RES) Resonance=MIN_RES; 
			if (Cutoff<MIN_CUTOFF) Cutoff=MIN_CUTOFF; 
	 
			if (n%FILTERGRAN==0)
			{
				for (nInd = 0; nInd < iir.length; nInd++) 
				{
					a2 = ProtoCoef[nInd].a2; 
		
					a0 = ProtoCoef[nInd].a0; 
					a1 = ProtoCoef[nInd].a1; 
	          
					b0 = ProtoCoef[nInd].b0; 
					b1 = ProtoCoef[nInd].b1 / Resonance;      
					b2 = ProtoCoef[nInd].b2; 	
					szxform(&a0, &a1, &a2, &b0, &b1, &b2, Cutoff*(Cutoff/1000.0f), fs, &k, coef); 
					coef += 4;                       

					iir.coef[0] = k; 
				
					m_LastQ=AsFloat(Q);
					m_LastFC=AsFloat(fc);			
				}
			}		  
		 		 
		 	out = iir_filter(in/0.5f,&iir);
		}	 

	 	SetOutput(0,n,out);
	}		
}
	
void FilterPlugin::Migrate(Spiral::Core::Description &d)
{
	if (d.Version() != 1)
		return;

	const int revc = d.Field(3, "RevC"), revr = d.Field(4, "RevR");
	if (revc >= 0) d.SetValue(revc, false);
	if (revr >= 0) d.SetValue(revr, false);
	d.SetValue(0, 2);
}

void FilterPlugin::SetupCoeffs()
{
	ProtoCoef[0].a0 = 1.0; 
	ProtoCoef[0].a1 = 0; 
	ProtoCoef[0].a2 = 0; 
	ProtoCoef[0].b0 = 1.0; 
	ProtoCoef[0].b1 = 0.765367; 
	ProtoCoef[0].b2 = 1.0; 

	ProtoCoef[1].a0 = 1.0; 
	ProtoCoef[1].a1 = 0; 
	ProtoCoef[1].a2 = 0; 
	ProtoCoef[1].b0 = 1.0; 
	ProtoCoef[1].b1 = 1.847759; 
	ProtoCoef[1].b2 = 1.0; 
} 
