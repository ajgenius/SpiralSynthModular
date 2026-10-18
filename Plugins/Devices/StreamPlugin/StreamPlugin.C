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
#include "StreamPlugin.h"
#include "SpiralIcon.xpm"
#include "NoteTable.h"
#include <stdio.h>
#include "RiffWav.h"

using namespace std;
using namespace Spiral::Core;

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


SpiralPlugin* SpiralPlugin_CreateInstance() { return new StreamPlugin; }

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DEVICE;
}


const char** SpiralPlugin_GetIcon() { return SpiralIcon_xpm; }

int SpiralPlugin_GetID() { return 0x0119; }

string SpiralPlugin_GetName()
{
	return "Stream";
}

string SpiralPlugin_GetGroupName() { return "Delay/Sampling"; }

}

///////////////////////////////////////////////////////

static const float TRIG_THRESH = 0.1;
static const float BUFSECONDS = 1.0f;

StreamPlugin::StreamPlugin() :
m_SampleRate (44100),
m_SampleSize (256),
m_SamplePos (-1),
m_Mode(STOPM)
{
	m_PluginInfo.Name = "Stream";
	m_PluginInfo.Width = 245;
	m_PluginInfo.Height = 160;
	m_PluginInfo.NumInputs = 3;
	m_PluginInfo.NumOutputs = 5;
	m_PluginInfo.PortTips.push_back ("Pitch CV");
	m_PluginInfo.PortTips.push_back ("Play Trigger");
	m_PluginInfo.PortTips.push_back ("Stop Trigger");
	m_PluginInfo.PortTips.push_back ("Left Out");
	m_PluginInfo.PortTips.push_back ("Right Out");
	m_PluginInfo.PortTips.push_back ("Finish Trigger");
	m_PluginInfo.PortTips.push_back ("Playing Trigger");
	m_PluginInfo.PortTips.push_back ("Position CV");
	AsFloat(m_Volume) = 1.0f;
	AsFloat(m_PitchMod) = 1.0f;
        m_GUIArgs.PlayOut = false;
        strcpy (m_GUIArgs.FileName, "");
	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 1);

	m_Volume = FloatProperty(DefaultLinearFlags, 1.0, 0, 2, 0.001, 0.01);
	m_PitchMod = FloatProperty(DefaultLinearFlags, 1.0, 0, 4, 0.001, 0.01);

	RegisterSharedProperty(m_Volume, "Volume", "Volume");
	RegisterSharedProperty(m_PitchMod, "Pitch", "Pitch Modulation");
     m_Pos = FloatProperty(Property::Internal, 0, 0, 0x7FFFFF, 1, 1);
     m_StreamPos = SignedProperty(Property::Internal, 0, 0, 0x7FFFFFFF, 1, 1);
     m_GlobalPos = FloatProperty(Property::Internal, 0, 0, 0x7FFFFF, 1, 1);
     m_Pitch = FloatProperty(Property::Internal, 1.0f, 0, 16, 0.001, 0.01);
     RegisterSharedProperty(m_Pos, "Pos", "Position in the chunk");
     RegisterSharedProperty(m_StreamPos, "StreamPos", "Chunk position in the file");
     RegisterSharedProperty(m_GlobalPos, "GlobalPos", "Position in the file");
     RegisterSharedProperty(m_Pitch, "Pitch Scale", "Sample rate ratio");
	m_AudioCH->RegisterData ("FileName", ChannelHandler::INPUT,
                                             &m_GUIArgs.FileName, sizeof (m_GUIArgs.FileName));
	m_AudioCH->RegisterData ("EchoFileName", ChannelHandler::OUTPUT,
                                             &m_GUIArgs.FileName, sizeof (m_GUIArgs.FileName));
	m_AudioCH->Register ("Time", &m_GUIArgs.Time);
	m_AudioCH->Register ("TimeOut", &m_GUIArgs.TimeOut, ChannelHandler::OUTPUT);
	m_AudioCH->Register ("MaxTime", &m_GUIArgs.MaxTime, ChannelHandler::OUTPUT);
	m_AudioCH->Register ("Playing", &m_GUIArgs.PlayOut, ChannelHandler::OUTPUT);
}

StreamPlugin::~StreamPlugin()
{
}

PluginInfo &StreamPlugin::Initialise(const HostInfo *Host)
{
	PluginInfo& Info = SpiralPlugin::Initialise(Host);
	return Info;
}



void StreamPlugin::Reset()
{
	ResetPorts();
	AsSigned(m_StreamPos) = 0;
	AsFloat(m_GlobalPos) = 0;
	m_SamplePos = -1;
	AsFloat(m_Pos) = 0;
}


void StreamPlugin::Execute() {
     if (m_File.IsOpen()) {
        for (int n=0; n<m_HostInfo->BUFSIZE; n++) {
            bool FinTrig = false;
            float CVPitch = GetInput(0, n)*10.0f;
            if (GetInput (1, n) > 0) m_Mode = PLAYM;
            if (GetInput (2, n) > 0) {
               m_Mode = STOPM;
               AsFloat(m_Pos) = 0;
               AsFloat(m_GlobalPos) = 0;
               AsSigned(m_StreamPos) = 0;
            }

            if (AsFloat(m_Pos)<0) {
               AsFloat(m_Pos) = m_SampleSize - 1;
               AsSigned(m_StreamPos) -= m_SampleSize;
               FinTrig = AsSigned(m_StreamPos) < 0;
               if (FinTrig) {
                  AsSigned(m_StreamPos) = m_File.GetSize() - m_SampleSize;
                  AsFloat(m_GlobalPos) = AsSigned(m_StreamPos);
               }
               m_File.SeekToChunk (AsSigned(m_StreamPos));

               if ((m_File.GetSize() - AsSigned(m_StreamPos)) < 256)
		 m_SampleSize = m_File.GetSize() - AsSigned(m_StreamPos);
 	       else
		 m_SampleSize = 256;

               m_File.LoadChunk (m_SampleSize, m_SampleL, m_SampleR);
            }
            else if (AsFloat(m_Pos) >= m_SampleSize) {
               AsFloat(m_Pos) = 0;
               AsSigned(m_StreamPos) += m_SampleSize;
               FinTrig = AsSigned(m_StreamPos) >= m_File.GetSize();
               if (FinTrig) {
                  AsSigned(m_StreamPos) = 0;
                  AsFloat(m_GlobalPos) = 0;
               }
               m_File.SeekToChunk (AsSigned(m_StreamPos));

               if ((m_File.GetSize() - AsSigned(m_StreamPos)) < 256)
		 m_SampleSize = m_File.GetSize() - AsSigned(m_StreamPos);
 	       else
		 m_SampleSize = 256;

               m_File.LoadChunk (m_SampleSize, m_SampleL, m_SampleR);
            }
            // finished trigger
            if (FinTrig) SetOutput (2, n, 1);
            else SetOutput (2, n, 0);
            if (m_Mode==PLAYM) {
              // left and right outputs
              SetOutput (0, n, m_SampleL[AsFloat(m_Pos)] * AsFloat(m_Volume));
              SetOutput (1, n, m_SampleR[AsFloat(m_Pos)] * AsFloat(m_Volume));
              // play trigger
              SetOutput (3, n, 1);
              AsFloat(m_Pos) += AsFloat(m_PitchMod) + CVPitch;
              AsFloat(m_GlobalPos) += AsFloat(m_PitchMod) + CVPitch;
              float  plop = AsFloat(m_GlobalPos) / (float)m_File.GetSize();
              SetOutput (4, n, plop);
            }
            else {
              SetOutput (0, n, 0);
              SetOutput (1, n, 0);
              SetOutput (3, n, 0);
              SetOutput (4, n, 0);
            }
        }
        m_GUIArgs.TimeOut = AsFloat(m_GlobalPos) / (float)m_SampleRate;
        m_GUIArgs.PlayOut = m_Mode==PLAYM;
     }
}

void StreamPlugin::ExecuteCommands() {
     if (m_AudioCH->IsCommandWaiting()) {
        switch (m_AudioCH->GetCommand()) {
	  case SET_TIME:
            SetTime();
            break;
          case LOAD:
            OpenStream();
            break;
	  case RESTART:
            AsSigned(m_StreamPos) = 0;
            AsFloat(m_GlobalPos) = 0;
            break;
	  case STOP:
            m_Mode = STOPM;
            break;
          case PLAY:
            m_Mode = PLAYM;
            break;
	}
     }
}

void StreamPlugin::SetTime (void) {
     AsFloat(m_GlobalPos) = m_SampleRate * m_GUIArgs.Time;
     AsSigned(m_StreamPos) = (int)(m_SampleRate * m_GUIArgs.Time);
     AsFloat(m_Pos) = m_SampleSize;
}

void StreamPlugin::OpenStream (void) {
     AsSigned(m_StreamPos) = 0;
     AsFloat(m_GlobalPos) = 0;

     if (m_File.IsOpen ()) m_File.Close ();
     m_File.Open (m_GUIArgs.FileName, WavFile::READ);

     if (m_File.GetSize() < 256)
       m_SampleSize = m_File.GetSize();
     else
       m_SampleSize = 256;

     m_SampleL.Allocate (m_SampleSize);
     m_SampleR.Allocate (m_SampleSize);
     AsFloat(m_Pitch) = m_SampleRate / (float)m_HostInfo->SAMPLERATE;
     if (m_File.IsStereo ()) {
        AsFloat(m_Pitch) *= 2;
        m_GUIArgs.MaxTime = GetLength();
     }
     else m_GUIArgs.MaxTime = GetLength() / 2;
}

float StreamPlugin::GetLength (void) {
      if (m_File.IsStereo()) return m_File.GetSize() / (float)m_File.GetSamplerate ();
                        else return m_File.GetSize() / (float)m_File.GetSamplerate () * 2;
}

void StreamPlugin::Describe(Spiral::Core::Description &d) {
     d.Value(m_Version).Separator(" ")
        .Value(AsFloat(m_Volume)).Separator(" ")
        .Value(AsFloat(m_PitchMod)).Separator(" ")
        .Value(strlen (m_GUIArgs.FileName)).Separator(" ")
        .Value(m_GUIArgs.FileName).Separator(" ")
        // is it really necessary to save this lot??
        .Value(AsFloat(m_Pos)).Separator(" ")
        .Value(AsSigned(m_StreamPos)).Separator(" ")
        .Value(AsFloat(m_GlobalPos)).Separator(" ")
        .Value(AsFloat(m_Pitch)).Separator(" ").Line();
}

void StreamPlugin::Apply(Spiral::Core::Description::Reader &r) {
     int version;
     r.Value(version);

     r.Value(AsFloat(m_Volume)).Value(AsFloat(m_PitchMod));
     int size;
     std::string FileName;
     r.Value(size);
     r.Value(FileName);   // the counted bytes, one value
     if (size > 255) size = 255;
     strncpy (m_GUIArgs.FileName, FileName.c_str(), size);
     m_GUIArgs.FileName[size] = 0;
     if (std::string(m_GUIArgs.FileName) != "None") OpenStream ();
       // is it really necessary to load this lot??
     r.Value(AsFloat(m_Pos));
     r.Value(AsSigned(m_StreamPos));
     r.Value(AsFloat(m_GlobalPos));
     r.Value(AsFloat(m_Pitch));
}

