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

// is this still relevant
// for lrintf()
#define	_ISOC9X_SOURCE	1
#define _ISOC99_SOURCE	1

//#include <math.h>
//#include <sys/types.h>
//#include <sys/ioctl.h>
//#include <unistd.h>
//#include <limits.h>
#include "DiskWriterPlugin.h"
#include "SpiralIcon.xpm"
#include <stdlib.h>

using namespace std;
using namespace Spiral::Core;

// what's this for - should it still be here?
#define CHECK_AND_REPORT_ERROR	if (result<0)         \
{                     \
	perror("Sound device did not accept settings"); \
	m_OutputOk=false; \
	return false;     \
}

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


SpiralPlugin* SpiralPlugin_CreateInstance() { return new DiskWriterPlugin; }

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DEVICE;
}


const char** SpiralPlugin_GetIcon() { return SpiralIcon_xpm; }

int SpiralPlugin_GetID() { return 41; }

string SpiralPlugin_GetName()
{
	return "DiskWriter";
}

string SpiralPlugin_GetGroupName() { return "InputOutput"; }

}

///////////////////////////////////////////////////////

const NumericPropertyValue BitRates[] = 
{
	DefaultUnsigned(16),
	DefaultUnsigned(24),
	DefaultUnsigned(32)
};

DiskWriterPlugin::DiskWriterPlugin()
{
	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 3);

	m_BitsPerSample = SetProperty(Property::WriteOnly, 0, PropertySet(BitRates, sizeof(BitRates)/sizeof(BitRates[0])));
	m_Stereo = ToggleProperty(Property::WriteOnly, true);
	m_Recording = ToggleProperty(Property::ReadOnly, false);
	m_TimeRecorded = FloatProperty(Property::ReadOnly, 0, 0, 0, 0, 0);
	RegisterSharedProperty(m_BitsPerSample, "Bits Per Sample", "Bit depth");
	RegisterSharedProperty(m_Stereo, "Stereo", "Stereo or mono");
	RegisterSharedProperty(m_Recording, "Recording", "Recording now");
	RegisterSharedProperty(m_TimeRecorded, "Time Recorded", "Seconds recorded");
	m_PluginInfo.Name="DiskWriter";
	m_PluginInfo.Width=160;
	m_PluginInfo.Height=115;
	m_PluginInfo.NumInputs=3;
	m_PluginInfo.NumOutputs=0;
	m_PluginInfo.PortTips.push_back("Left Out");
	m_PluginInfo.PortTips.push_back("Right Out");
	m_PluginInfo.PortTips.push_back("Record Controller");

        AsSigned(m_BitsPerSample) = 16;
        AsBoolean(m_Stereo) = true;
	AsBoolean(m_Recording) = false;
	AsFloat(m_TimeRecorded) = 0;

	m_Version = 3;

	m_AudioCH->RegisterData("Filename",ChannelHandler::INPUT,m_GUIArgs.Name,256);
}

DiskWriterPlugin::~DiskWriterPlugin()
{
}

PluginInfo &DiskWriterPlugin::Initialise(const HostInfo *Host)
{
	PluginInfo& Info= SpiralPlugin::Initialise(Host);
	return Info;
}



void DiskWriterPlugin::Execute()
{
	if(AsBoolean(m_Recording) && m_Wav.IsOpen())
	{
		int on=0;
		float LeftBuffer[GetHostInfo()->BUFSIZE], RightBuffer[GetHostInfo()->BUFSIZE];

		for (int n=0; n<GetHostInfo()->BUFSIZE; n++)
		{
			// stereo channels - interleave
			LeftBuffer[n]=GetInput(0,n);
			RightBuffer[n]=GetInput(1,n);
		}

		m_Wav.Save(LeftBuffer, RightBuffer, GetHostInfo()->BUFSIZE);
		AsFloat(m_TimeRecorded) = m_Wav.GetSize()/m_Wav.GetSamplerate();
	}
}

void DiskWriterPlugin::ExecuteCommands()
{
	if (m_AudioCH->IsCommandWaiting())
	{
		switch(m_AudioCH->GetCommand())
		{
			case OPENWAV :
				if (m_Wav.GetSamplerate() != GetHostInfo()->SAMPLERATE) {
					m_Wav.SetSamplerate(GetHostInfo()->SAMPLERATE);
				}
				if (m_Wav.GetBitsPerSample() != AsSigned(m_BitsPerSample)) {
					m_Wav.SetBitsPerSample(AsSigned(m_BitsPerSample));
				}
				m_Wav.Open(m_GUIArgs.Name,WavFile::WRITE, (AsBoolean(m_Stereo))?(WavFile::STEREO):(WavFile::MONO));
				AsFloat(m_TimeRecorded) = 0;
			break;
			case CLOSEWAV : m_Wav.Close(); break;
			case RECORD : AsBoolean(m_Recording)=true; break;
			case STOP : AsBoolean(m_Recording)=false;	break;
			default : break;
		}
	}
}

void DiskWriterPlugin::Describe(Spiral::Core::Description &d)
{
	d.Value(DeviceVersion).Separator(" ").Value(m_BitsPerSample).Separator(" ").Value(m_Stereo).Separator(" ");
}

void DiskWriterPlugin::Apply(Spiral::Core::Description::Reader &r)
{
	int Version;

	// originally DiskWriter had NO streaming code whatsover
	// so an old patch has no state for it at all
	if (r.More())
	{
		r.Value(Version);
	}
	else
	{
		//No Version, so use Version 1
		Version = 1;
	}

	switch (Version)
	{
		case 3:
		{
			r.Value(m_BitsPerSample).Value(m_Stereo);
		}
		break;

		case 1:
		{
			//use original fixed defaults
			AsSigned(m_BitsPerSample) = 16;
			AsBoolean(m_Stereo) = true;
		}
		break;
	}
}

void DiskWriterPlugin::Migrate(Spiral::Core::Description &d)
{
	if (d.Version() != 2)
		return;

	int bits = d.Field(1, "Bits Per Sample");
	if (bits >= 0)
	{
		int depth = atoi(d.Values()[bits].c_str());
		d.SetValue(bits, depth < 24 ? 16 : depth < 32 ? 24 : 32);
	}

	d.SetValue(0, 3);
}
