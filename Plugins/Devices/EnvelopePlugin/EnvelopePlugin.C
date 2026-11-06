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
#include "EnvelopePlugin.h"
#include "SpiralIcon.xpm"

using namespace std;
using namespace Spiral::Core;

static const FloatType TIMED_SLIDER_MAX = 3.0f;

// a bit of a crap filter to smooth clicks
static float SMOOTH = 0.99;
static float ONEMINUS_SMOOTH = 1-SMOOTH;

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
	return new EnvelopePlugin;
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

EnvelopePlugin::EnvelopePlugin() :
m_Trigger(false),
m_t(-1.0f),
m_Current(0)
{
	m_PluginInfo.Name="Envelope";
	m_PluginInfo.Width=142;
	m_PluginInfo.Height=132;
	m_PluginInfo.NumInputs=8;
	m_PluginInfo.NumOutputs=2;
	m_PluginInfo.PortTips.push_back("Trigger CV");	
	m_PluginInfo.PortTips.push_back("Input");	
	m_PluginInfo.PortTips.push_back("Threshold CV");
	m_PluginInfo.PortTips.push_back("Attack CV");
	m_PluginInfo.PortTips.push_back("Decay CV");
	m_PluginInfo.PortTips.push_back("Sustain CV");
	m_PluginInfo.PortTips.push_back("Release CV");
	m_PluginInfo.PortTips.push_back("Volume CV");
	m_PluginInfo.PortTips.push_back("CV");	
	m_PluginInfo.PortTips.push_back("Output");
	
	SetUnsigned(DeviceVersion, 2);

	m_Attack = FloatProperty(DefaultLinearFlags, 0.0, 0, TIMED_SLIDER_MAX, 0.0001, 0.001);
	m_Decay = FloatProperty(DefaultLinearFlags, 0.5, 0, TIMED_SLIDER_MAX, 0.0001, 0.001);
	m_Sustain = FloatProperty(DefaultLinearFlags, 1.0, 0, 1, 0.0001, 0.001);
	m_Release = FloatProperty(DefaultLinearFlags, 1.0, 0, TIMED_SLIDER_MAX, 0.0001, 0.001);
	m_TrigThresh = FloatProperty(DefaultLinearFlags, 0.01, 0, 1, 0.0001, 0.001);
	m_Volume = FloatProperty(DefaultLinearFlags, 0.5, 0, 1, 0.0001, 0.001);

	RegisterSharedProperty(m_Attack, "Attack", "Attack");
	RegisterSharedProperty(m_Decay, "Decay", "Decay");
	RegisterSharedProperty(m_Sustain, "Sustain", "Sustain");
	RegisterSharedProperty(m_Release, "Release", "Release");
	RegisterSharedProperty(m_Volume, "Volume", "Volume");
	RegisterSharedProperty(m_TrigThresh, "Threshold", "Threshold");
}

EnvelopePlugin::~EnvelopePlugin()
{
}

PluginInfo &EnvelopePlugin::Initialise(const HostInfo *Host)
{	
	PluginInfo& Info = SpiralPlugin::Initialise(Host);
	m_SampleTime=1.0/(float)(m_HostInfo->SAMPLERATE);
	return Info;
}

void EnvelopePlugin::Reset()
{
	ResetPorts();
	m_SampleTime=1.0/(float)(m_HostInfo->SAMPLERATE);
	m_Trigger = false;
	m_t = -1.0f;
}

void EnvelopePlugin::Execute()
{
	float temp=0, trigthresh, attack, decay, sustain, release, volume;
	bool Freeze=false;
	
	trigthresh = AsFloat(m_TrigThresh);
	attack = AsFloat(m_Attack);
	decay = AsFloat(m_Decay);
	sustain = AsFloat(m_Sustain);
	release = AsFloat(m_Release);
	volume = AsFloat(m_Volume);

	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{	
		float t=trigthresh, a=attack, d=decay, s=sustain, r=release, v=volume;

		if (InputExists(2)) t += GetInput(2,n);
		if (InputExists(3)) a += GetInput(3,n);
		if (InputExists(4)) d += GetInput(4,n);
		if (InputExists(5)) s += GetInput(5,n);
		if (InputExists(6)) r += GetInput(6,n);
		if (InputExists(7)) v += GetInput(7,n);
		
		// Check the trigger CV values
		if (GetInput(0,n)>t)
		{
			if (m_Trigger==false) 
			{
				m_t=0.0f; 
				m_Trigger=true;
			}
		}
		else
		{	
			m_Trigger=false;
		}
		
		// if we are in the envelope...
		if (m_t>=0 && m_t<a+d+r) 
		{				
			// find out what part of the envelope we are in	
			// in the attack
			if (m_t<a)
			{			
				// get normalised position to
				// get the volume between 0 and 1
				temp=m_t/a;
			}
			else
			// in the decay
			if (m_t<a+d)
			{
				//if (n==0) cerr<<"in decay"<<endl;
				// normalised position in m_Attack->m_Decay range
				float nt=(m_t-a)/d;
				
				// volume between 1 and m_Sustain
				temp=(1-nt)+(s*nt);
			}
			else
			// in the release
			{
				//if (n==0) cerr<<"in release"<<endl;
				// normalised position in m_Decay->m_Release range
				float nt=(m_t-(a+d))/r;
				
				// volume between m_Sustain and 0			
				temp=s*(1-nt);
				
				if (r<0.2f)
				{
					temp=s;
				}	
				
				if (m_Trigger) Freeze=true;
			}
			
			temp*=v;
			
			if (!feq(temp,m_Current,0.01)) 
			{
				// only filter if necc
				temp=(temp*ONEMINUS_SMOOTH+m_Current*SMOOTH);
			}
			
			SetOutput(0,n,temp);
			SetOutput(1,n,GetInput(1,n)*temp); 
			m_Current=temp;
				
			if (!Freeze) m_t+=m_SampleTime;
		}
		else
		{
			if (!feq(temp,m_Current,0.01)) 
			{
				temp=m_Current*SMOOTH;
			}
			
			SetOutput(0,n,temp);
			SetOutput(1,n,GetInput(1,n)*temp); 
			m_Current=temp;
			
			// if we've run off the end
			if (m_t>a+d+r)
			{
				m_t=-1;
				return;
			}

		}
	} 	
}
