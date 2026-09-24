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
#include "AmpPlugin.h"
#include "SpiralIcon.xpm"

#define PI 3.141592654

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
	return new AmpPlugin;
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
	return 0x0009;
}

string SpiralPlugin_GetGroupName()
{
	return "Amps/Mixers";
}
}

///////////////////////////////////////////////////////

AmpPlugin::AmpPlugin() :
m_Gain(1.0f),
m_DC(0.0f)
{
	m_PluginInfo.Name="Amp";
	m_PluginInfo.Width = 120;
	m_PluginInfo.Height = 140;
	m_PluginInfo.NumInputs=3;
	m_PluginInfo.NumOutputs=1;
	m_PluginInfo.PortTips.push_back("Input");
	m_PluginInfo.PortTips.push_back("Gain CV");
	m_PluginInfo.PortTips.push_back("DC Offset CV");
	m_PluginInfo.PortTips.push_back("Output");
	m_AudioCH->Register("Gain",&m_Gain);
	m_AudioCH->Register("DC",&m_DC);
}

AmpPlugin::~AmpPlugin()
{
}

PluginInfo &AmpPlugin::Initialise(const HostInfo *Host)
{
	return SpiralPlugin::Initialise(Host);
}



void AmpPlugin::Execute()
{
	float in;
	
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		in = GetInput(0,n);
		in *= m_Gain+GetInput(1,n);
		in += (-m_DC)+GetInput(2,n);
		SetOutput(0,n,in);
                //cerr << m_Gain << " ";	 
	}		
}
	
void AmpPlugin::Randomise()
{
}
	
void AmpPlugin::StreamOut(ostream &s)
{
	s<<m_Version<<" "<<m_Gain<<" "<<m_DC<<" ";
}

void AmpPlugin::StreamIn(istream &s)
{	
	int version;
	s>>version;
	s>>m_Gain>>m_DC;
}
