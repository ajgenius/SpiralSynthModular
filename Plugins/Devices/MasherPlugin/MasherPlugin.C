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
#include <cstdlib>
#include "MasherPlugin.h"
#include "SpiralIcon.xpm"

#define PI 3.141592654

using namespace std;
using namespace Spiral::Core;

float RandRange(float L, float H)
{
	return ((rand()%10000/10000.0f)*(H-L))+L;
}

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
	return new MasherPlugin;
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
	return 54;
}

string SpiralPlugin_GetName()
{
	return "Masher";
}

string SpiralPlugin_GetGroupName()
{
	return "Filters/FX";
}
}

///////////////////////////////////////////////////////

MasherPlugin::MasherPlugin() :
m_ReadGrain(0),
m_WriteGrain(0)
{
	m_PluginInfo.Name="Masher";
	m_PluginInfo.Width=120;
	m_PluginInfo.Height=140;
	m_PluginInfo.NumInputs=3;
	m_PluginInfo.NumOutputs=1;
	m_PluginInfo.PortTips.push_back("Input");
	m_PluginInfo.PortTips.push_back("GrainPitch");
	m_PluginInfo.PortTips.push_back("Density");
	m_PluginInfo.PortTips.push_back("Output");
	
	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 1);

	m_GrainPitch = FloatProperty(DefaultLinearFlags,1,0,5,0.001,0.01);
	RegisterSharedProperty(m_GrainPitch, "GrainPitch", "GrainPitch");
	m_GrainStoreSize = SignedProperty(DefaultLinearFlags,MAX_GRAINSTORE_SIZE, 0, MAX_GRAINSTORE_SIZE, 1, 1);
	RegisterSharedProperty(m_GrainStoreSize, "GrainStoreSize", "GrainStoreSize");
	m_Density = SignedProperty(DefaultLinearFlags,10, 0, MAX_GRAINSTORE_SIZE, 1, 1);
	RegisterSharedProperty(m_Density, "Density", "Density");
	m_Randomness = SignedProperty(DefaultLinearFlags,0,0,100,1,1);
	RegisterSharedProperty(m_Randomness, "Randomness", "Randomness");
	
	
	
	
}

MasherPlugin::~MasherPlugin()
{
}

PluginInfo &MasherPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);
}



void MixPitch(Sample &src, Sample &dst, int Pos, float Pitch)
{
	float n=0; 
	int p=Pos;
	while (n<src.GetLength() && p<dst.GetLength())
	{
		if (p>=0) dst.Set(p,dst[p]+src[n]);
		n+=Pitch;
		p++;
	}
}

void MasherPlugin::Reset()
{
	ResetPorts();
	m_ReadGrain = 0;
	m_WriteGrain = 0;
}


void MasherPlugin::Execute()
{
	GetOutputBuf(0)->Zero();
	if (!InputExists(0)) return;
	
	int n=0;
	float s=GetInput(0,0);
	int last=0;
	bool First=true;
	
	// paste any overlapping grains to the start of the buffer.
	for (vector<GrainDesc>::iterator i = m_OverlapVec.begin();
		 i!=m_OverlapVec.end(); i++)
	{
		MixPitch(m_GrainStore[i->Grain],*GetOutputBuf(0),i->Pos-m_HostInfo->BUFSIZE,AsFloat(m_GrainPitch));
	}
	
	m_OverlapVec.clear();
	
	// chop up the buffer and put the grains in the grainstore	
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		if ((s<0 && GetInput(0,n)>0) || (s>0 && GetInput(0,n)<0))
		{
			// chop the bits between zero crossings
			if (!First) 
			{
				GetInput(0)->GetRegion(m_GrainStore[m_WriteGrain%AsSigned(m_GrainStoreSize)],last,n);
				m_WriteGrain++;
			}
			else First=false;
			
			last=n;
			s=GetInput(0,n);
		}
	}
		
	int NextGrain=0;
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		int Density = AsSigned(m_Density);
		if (InputExists(2)) Density=(int)(Density*GetInput(2,n));
		
		if(n>=NextGrain || rand()%1000<Density)
		{
			int GrainNum = m_ReadGrain%AsSigned(m_GrainStoreSize);
			float Pitch=AsFloat(m_GrainPitch);
			if (InputExists(1)) Pitch*=fabs(GetInput(1,n));
			MixPitch(m_GrainStore[GrainNum],*GetOutputBuf(0),n,Pitch);
			int GrainLength = (int)(m_GrainStore[GrainNum].GetLength()*Pitch);
			
			NextGrain=n+m_GrainStore[GrainNum].GetLength();
			
			// if this grain overlaps the buffer
			if (n+GrainLength>m_HostInfo->BUFSIZE)
			{
				GrainDesc new_grain;
				new_grain.Pos=n;
				new_grain.Grain=GrainNum;
				m_OverlapVec.push_back(new_grain);
			}
			
			if (AsSigned(m_Randomness)) m_ReadGrain+=1+rand()%AsSigned(m_Randomness);
			else m_ReadGrain++;
		}
	}		
}
	
void MasherPlugin::Randomise()
{
}
	
void MasherPlugin::Describe(Spiral::Core::Description &d)
{
	d.Value(m_Version).Separator(" ");
	d.Value(AsSigned(m_GrainStoreSize)).Separator(" ").Value(AsSigned(m_Density)).Separator(" ").Value(AsSigned(m_Randomness)).Separator(" ").Value(AsFloat(m_GrainPitch)).Separator(" ");
}

void MasherPlugin::Apply(Spiral::Core::Description::Reader &r)
{	
	int version;
	r.Value(version);
	r.Value(AsSigned(m_GrainStoreSize)).Value(AsSigned(m_Density)).Value(AsSigned(m_Randomness)).Value(AsFloat(m_GrainPitch));
}
