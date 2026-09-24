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
#include "RingModPlugin.h"
#include "SpiralIcon.xpm"

using namespace std;

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
	return new RingModPlugin;
}

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DSP;
}


const char** SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}

int SpiralPlugin_GetID()
{
	return 0x000a;
}

string SpiralPlugin_GetGroupName()
{
	return "Filters/FX";
}
}

///////////////////////////////////////////////////////

RingModPlugin::RingModPlugin() :
m_Amount(1.0f)
{
	m_PluginInfo.Name="Ring Mod";
	m_PluginInfo.Width=80;
	m_PluginInfo.Height=80;
	m_PluginInfo.NumInputs=2;
	m_PluginInfo.NumOutputs=1;
	m_PluginInfo.PortTips.push_back("Input 1");	
	m_PluginInfo.PortTips.push_back("Input 2");	
	m_PluginInfo.PortTips.push_back("Output");	
	
	m_AudioCH->Register("Amount",&m_Amount);
}

RingModPlugin::~RingModPlugin()
{
}

PluginInfo &RingModPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);
}



void RingModPlugin::Execute()
{
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		SetOutput(0,n,GetInput(0,n)*GetInput(1,n)*m_Amount);
	}		
}
	
void RingModPlugin::Randomise()
{
}
	
void RingModPlugin::StreamOut(ostream &s)
{
	s<<m_Version<<" "<<m_Amount<<" ";
}

void RingModPlugin::StreamIn(istream &s)
{	
	int version;
	s>>version;
	s>>m_Amount;
}

