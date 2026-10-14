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
#include "PoshSamplerPlugin.h"
#include "SpiralIcon.xpm"
#include "NoteTable.h"
#include "RiffWav.h"
#include <stdio.h>
#include <stdlib.h>

using namespace std;
using namespace Spiral::Core;

static const int NOTETRIG = NUM_SAMPLES * 2 + 1;
static const int REC_INPUT = 16;
static const int S1_INPUT = 18;
static const int S2_INPUT = 19;
static const int S3_INPUT = 20;

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

   SpiralPlugin* SpiralPlugin_CreateInstance() { return new PoshSamplerPlugin; }

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DEVICE;
}

   const char** SpiralPlugin_GetIcon() { return SpiralIcon_xpm; }
   int SpiralPlugin_GetID() { return 32; }
   string SpiralPlugin_GetName()
   {
       return "PoshSampler";
   }

   string SpiralPlugin_GetGroupName() { return "Delay/Sampling"; }
}

///////////////////////////////////////////////////////

PoshSamplerPlugin::PoshSamplerPlugin() :
m_Recording(false)
{
        m_PluginInfo.Name = "PoshSampler";
        m_PluginInfo.Width = 400;
        m_PluginInfo.Height = 215;
        m_PluginInfo.NumInputs = 21;
        m_PluginInfo.NumOutputs = 9;
        m_PluginInfo.PortTips.push_back ("Sample 1 Pitch");
	m_PluginInfo.PortTips.push_back ("Sample 1 Trigger");
        m_PluginInfo.PortTips.push_back ("Sample 2 Pitch");
        m_PluginInfo.PortTips.push_back ("Sample 2 Trigger");
        m_PluginInfo.PortTips.push_back ("Sample 3 Pitch");
        m_PluginInfo.PortTips.push_back ("Sample 3 Trigger");
        m_PluginInfo.PortTips.push_back ("Sample 4 Pitch");
        m_PluginInfo.PortTips.push_back ("Sample 4 Trigger");
        m_PluginInfo.PortTips.push_back ("Sample 5 Pitch");
        m_PluginInfo.PortTips.push_back ("Sample 5 Trigger");
        m_PluginInfo.PortTips.push_back ("Sample 6 Pitch");
        m_PluginInfo.PortTips.push_back ("Sample 6 Trigger");
        m_PluginInfo.PortTips.push_back ("Sample 7 Pitch");
        m_PluginInfo.PortTips.push_back ("Sample 7 Trigger");
        m_PluginInfo.PortTips.push_back ("Sample 8 Pitch");
        m_PluginInfo.PortTips.push_back ("Sample 8 Trigger");
        m_PluginInfo.PortTips.push_back ("Input");
        m_PluginInfo.PortTips.push_back ("Sample trigger pitch");
        m_PluginInfo.PortTips.push_back ("Sample 1 Start Pos");
        m_PluginInfo.PortTips.push_back ("Sample 2 Start Pos");
        m_PluginInfo.PortTips.push_back ("Sample 3 Start Pos");
        m_PluginInfo.PortTips.push_back ("Mixed Output");
        m_PluginInfo.PortTips.push_back ("Sample 1 Output");
        m_PluginInfo.PortTips.push_back ("Sample 2 Output");
        m_PluginInfo.PortTips.push_back ("Sample 3 Output");
        m_PluginInfo.PortTips.push_back ("Sample 4 Output");
        m_PluginInfo.PortTips.push_back ("Sample 5 Output");
        m_PluginInfo.PortTips.push_back ("Sample 6 Output");
        m_PluginInfo.PortTips.push_back ("Sample 7 Output");
        m_PluginInfo.PortTips.push_back ("Sample 8 Output");
        m_Samples = VectorProperty (Property::WriteOnly);

        for (int n=0; n<NUM_SAMPLES; n++)
            m_Samples.Add (NewSampleSettings (n));

        RegisterSharedProperty (m_Samples, "Samples", "Per-sample settings");

        for (int n=0; n<NUM_SAMPLES; n++) {
            Sample* NewSample = new Sample;
            m_SampleVec.push_back (NewSample);
            SampleDesc* NewDesc = new SampleDesc;
            InitializeSampleDescription (NewDesc, n, n);
            m_SampleDescVec.push_back (NewDesc);
        }
        m_Version = 4;
        SetUnsigned(DeviceVersion, 4);
        m_Current = 0;
        m_AudioCH->Register ("Num", &m_GUIArgs.Num);
        m_AudioCH->Register ("Value", &m_GUIArgs.Value);
        m_AudioCH->Register ("Bool", &m_GUIArgs.Boole);
        m_AudioCH->Register ("Int", &m_GUIArgs.Int);
        m_AudioCH->Register ("Start", &m_GUIArgs.Start);
        m_AudioCH->Register ("End", &m_GUIArgs.End);
        m_AudioCH->Register ("LoopStart", &m_GUIArgs.LoopStart);
        m_AudioCH->RegisterData ("Name", ChannelHandler::INPUT, &m_GUIArgs.Name, sizeof (m_GUIArgs.Name));
        m_AudioCH->Register ("PlayPos", &m_CurrentPlayPos, ChannelHandler::OUTPUT);
        m_AudioCH->RegisterData ("SampleBuffer", ChannelHandler::OUTPUT_REQUEST, &m_SampleBuffer, TRANSBUF_SIZE);
        m_AudioCH->Register ("SampleSize", &m_SampleSize, ChannelHandler::OUTPUT_REQUEST);
        m_AudioCH->Register ("BoolEcho", &m_GUIArgs.BoolEcho, ChannelHandler::OUTPUT);
        m_AudioCH->Register ("ValEcho", &m_GUIArgs.ValEcho, ChannelHandler::OUTPUT);
        m_AudioCH->Register ("IntEcho", &m_GUIArgs.IntEcho, ChannelHandler::OUTPUT);
}

PoshSamplerPlugin::~PoshSamplerPlugin() {
   for (vector<Sample*>::iterator i=m_SampleVec.begin(); i!=m_SampleVec.end(); i++)
       delete(*i);
   for (vector<SampleDesc*>::iterator i=m_SampleDescVec.begin(); i!=m_SampleDescVec.end(); i++)
       delete(*i);
}

PluginInfo &PoshSamplerPlugin::Initialise (const HostInfo *Host) {
           return SpiralPlugin::Initialise (Host);
}



void PoshSamplerPlugin::Reset() {
     ResetPorts();
     m_Current = 0;
     for (int s=0; s<NUM_SAMPLES; s++) {
         m_SampleDescVec[s]->Pitch = m_InitialPitch[s] * m_SampleDescVec[s]->SampleRate / (float)m_HostInfo->SAMPLERATE;
         SampleSigned (s, SLOT_LOOPEND) = m_SampleVec[s]->GetLength() - 1;
     }
}

void PoshSamplerPlugin::Execute() {
     static bool Pong = false;
     for (int s=0; s<NUM_SAMPLES+1; s++) GetOutputBuf (s)->Zero();
     float Freq = 0;
     for (int n=0; n<m_HostInfo->BUFSIZE; n++) {
         Freq = GetInputPitch (NOTETRIG, n);
         for (int s=0; s<NUM_SAMPLES; s++) {
             SampleDesc *S = m_SampleDescVec[s];
             // if we have a sample here
             if (m_SampleVec[s]->GetLength()) {
                // Convert the CV input into a useable trigger
                if (SampleBool (s, SLOT_RETRIG) || (SampleFloat (s, SLOT_SAMPLEPOS) == -1)) {
                   if (GetInput (s*2 + 1, n) > 0 || feq (Freq, NoteTable[SampleSigned (s, SLOT_NOTE)], 0.01f)) {
                      if (S->TriggerUp) {
                         if (s==0 && InputExists (S1_INPUT))
                            SampleSigned (s, SLOT_PLAYSTART) = (long int)((GetInput (S1_INPUT, n) * 0.5 + 0.5f) * (SampleSigned (s, SLOT_LOOPEND)-SampleSigned (s, SLOT_LOOPSTART))) + SampleSigned (s, SLOT_LOOPSTART);
                         if (s==1 && InputExists (S2_INPUT))
                            SampleSigned (s, SLOT_PLAYSTART) = (long int)((GetInput (S2_INPUT, n) * 0.5 + 0.5f) * (SampleSigned (s, SLOT_LOOPEND)-SampleSigned (s, SLOT_LOOPSTART))) + SampleSigned (s, SLOT_LOOPSTART);
                         if (s==2 && InputExists (S3_INPUT))
                            SampleSigned (s, SLOT_PLAYSTART) = (long int)((GetInput (S3_INPUT, n) * 0.5 + 0.5f) * (SampleSigned (s, SLOT_LOOPEND)-SampleSigned (s, SLOT_LOOPSTART))) + SampleSigned (s, SLOT_LOOPSTART);
                         if (SampleSigned (s, SLOT_PLAYSTART) < 0) SampleSigned (s, SLOT_PLAYSTART) = 0;
                         SampleFloat (s, SLOT_SAMPLEPOS) = SampleSigned (s, SLOT_PLAYSTART);
                         S->TriggerUp = false;
                         S->Velocity = GetInput (s * 2 + 1, n);
                      }
                   }
                   else {
                      S->TriggerUp = true;
                      // end it if it's looping
                      if (SampleBool (s, SLOT_LOOP)) SampleFloat (s, SLOT_SAMPLEPOS) = -1;
                   }
                }
                // if the sample has ended
                if (SampleFloat (s, SLOT_SAMPLEPOS) >= SampleSigned (s, SLOT_LOOPEND) || SampleFloat (s, SLOT_SAMPLEPOS) >= m_SampleVec[s]->GetLength()) {
                   if (SampleBool (s, SLOT_LOOP)) {
                      if (SampleBool (s, SLOT_PINGPONG)) Pong = true;
                      else SampleFloat (s, SLOT_SAMPLEPOS) = SampleSigned (s, SLOT_LOOPSTART);
                   }
                   else {
                      SampleFloat (s, SLOT_SAMPLEPOS) = -1;
                   }
                }
                // if the sample has ended ponging
                if (Pong && SampleFloat (s, SLOT_SAMPLEPOS) <= SampleSigned (s, SLOT_LOOPSTART))
                   Pong = false;
                if (SampleFloat (s, SLOT_SAMPLEPOS) != -1) {
                   if (InputExists (s * 2)) {
                      // Get the pitch from the CV
                      float PlayFreq = GetInputPitch (s * 2, n);
                      // assumtion: base frequency = 440 (middle A)
                      S->Pitch = PlayFreq / 440;
                      S->Pitch *= S->SampleRate / (float)m_HostInfo->SAMPLERATE;
                   }
                   // mix the sample to the output.
                   MixOutput (0, n, (*m_SampleVec[s])[SampleFloat (s, SLOT_SAMPLEPOS)] * SampleFloat (s, SLOT_VOLUME) * S->Velocity);
                   // copy the sample to it's individual output.
                   SetOutput (s + 1, n, ((*m_SampleVec[s])[SampleFloat (s, SLOT_SAMPLEPOS)] * SampleFloat (s, SLOT_VOLUME)));
                   float Freq = S->Pitch;
                   if (SampleSigned (s, SLOT_OCTAVE) > 0) Freq *= 1 << (SampleSigned (s, SLOT_OCTAVE));
                   if (SampleSigned (s, SLOT_OCTAVE) < 0) Freq /= 1 << (-SampleSigned (s, SLOT_OCTAVE));
                   if (Pong) SampleFloat (s, SLOT_SAMPLEPOS) -= Freq * SampleFloat (s, SLOT_PITCHMOD);
                   else SampleFloat (s, SLOT_SAMPLEPOS) += Freq * SampleFloat (s, SLOT_PITCHMOD);
                }
             }
         }
     }
     // record
     static int LastRecording = false;
     if (m_Recording && InputExists (REC_INPUT)) {
        int s = 0; //GUI->GetCurrentSample();
        if (!LastRecording) m_SampleVec[s]->Clear();
        // new sample
        if (m_SampleVec[s]->GetLength() == 0) {
           *m_SampleVec[s] =* GetInput (REC_INPUT);
           m_SampleDescVec[s]->SampleRate = m_HostInfo->SAMPLERATE;
           m_SampleDescVec[s]->Stereo = false;
           m_SampleDescVec[s]->Pitch *= 1.0f;
           m_InitialPitch[s] = m_SampleDescVec[s]->Pitch;
           SampleSigned (s, SLOT_LOOPEND) = m_SampleVec[s]->GetLength();
        }
        else {
           m_SampleVec[s]->Add (*GetInput (REC_INPUT));
           SampleSigned (s, SLOT_LOOPEND) = m_SampleVec[s]->GetLength();
        }
     }
     LastRecording = m_Recording;
     if (SampleFloat (m_Current, SLOT_SAMPLEPOS) > 0) {
        m_CurrentPlayPos = (long)SampleFloat (m_Current, SLOT_SAMPLEPOS);
     }
}

void PoshSamplerPlugin::ExecuteCommands() {
     if (m_AudioCH->IsCommandWaiting()) {
        switch (m_AudioCH->GetCommand()) {
          case LOAD:
             LoadSample (m_GUIArgs.Num, m_GUIArgs.Name);
             break;
          case SAVE:
             SaveSample (m_GUIArgs.Num, m_GUIArgs.Name);
             break;
          case SETVOL:
             SampleFloat (m_GUIArgs.Num, SLOT_VOLUME)=m_GUIArgs.Value;
             break;
          case SETPITCH:
             SampleFloat (m_GUIArgs.Num, SLOT_PITCHMOD)=m_GUIArgs.Value;
             break;
          case SETLOOP:
             SampleBool (m_GUIArgs.Num, SLOT_LOOP)=m_GUIArgs.Boole;
             break;
          case SETPING:
             SampleBool (m_GUIArgs.Num, SLOT_PINGPONG)=m_GUIArgs.Boole;
             break;
          case SETNOTE:
             SampleSigned (m_GUIArgs.Num, SLOT_NOTE)=m_GUIArgs.Int;
             break;
          case SETOCT:
             SampleSigned (m_GUIArgs.Num, SLOT_OCTAVE)=m_GUIArgs.Int-6;
             break;
          case SETPLAYPOINTS:
             SampleSigned (m_GUIArgs.Num, SLOT_PLAYSTART)=m_GUIArgs.Start;
             SampleSigned (m_GUIArgs.Num, SLOT_LOOPSTART)=m_GUIArgs.LoopStart;
             SampleSigned (m_GUIArgs.Num, SLOT_LOOPEND)=m_GUIArgs.End;
             break;
          case SETREC:
             m_Recording=m_GUIArgs.Boole;
             break;
          case CUT:
             Cut (m_GUIArgs.Num, m_GUIArgs.Start, m_GUIArgs.End);
             break;
          case COPY:
             Copy (m_GUIArgs.Num, m_GUIArgs.Start, m_GUIArgs.End);
             break;
          case PASTE:
             Paste (m_GUIArgs.Num, m_GUIArgs.Start, m_GUIArgs.End);
             break;
          case MIX:
             Mix (m_GUIArgs.Num, m_GUIArgs.Start, m_GUIArgs.End);
             break;
          case CROP:
             Crop (m_GUIArgs.Num, m_GUIArgs.Start, m_GUIArgs.End);
             break;
          case REV:
             Reverse (m_GUIArgs.Num, m_GUIArgs.Start, m_GUIArgs.End);
             break;
          case AMP:
             Amp (m_GUIArgs.Num, m_GUIArgs.Start, m_GUIArgs.End);
             break;
          case SETCURRENT:
             m_Current = m_GUIArgs.Num;
             break;
          case GETSAMPLE:
             m_AudioCH->SetupBulkTransfer ((void*)m_SampleVec[m_Current]->GetBuffer());
             m_SampleSize = m_SampleVec[m_Current]->GetLengthInBytes();
             break;
          case SETRETRIG:
             SampleBool (m_GUIArgs.Num, SLOT_RETRIG) = m_GUIArgs.Boole;
             break;
          case GETLOOP:
             m_GUIArgs.BoolEcho = SampleBool (m_Current, SLOT_LOOP);
             break;
          case GETPING:
             m_GUIArgs.BoolEcho = SampleBool (m_Current, SLOT_PINGPONG);
             break;
          case GETRETRIG:
             m_GUIArgs.BoolEcho = SampleBool (m_Current, SLOT_RETRIG);
             break;
          case GETVOL:
             m_GUIArgs.ValEcho = SampleFloat (m_Current, SLOT_VOLUME);
             break;
          case GETPITCH:
             m_GUIArgs.ValEcho = SampleFloat (m_Current, SLOT_PITCHMOD);
             break;
          case GETOCT:
             m_GUIArgs.IntEcho = SampleSigned (m_Current, SLOT_OCTAVE) + 6;
             break;
          case GETNOTE:
             m_GUIArgs.IntEcho = SampleSigned (m_Current, SLOT_NOTE);
             break;
        };
     }
}

VectorProperty *PoshSamplerPlugin::NewSampleSettings (int Note)
{
	VectorProperty *s = new VectorProperty (Property::WriteOnly);

	s->Add (new FloatProperty  (DefaultLinearFlags, 1.0f, 0.0f, 2.0f, 0.001f, 0.1f));
	s->Add (new FloatProperty  (DefaultLinearFlags, 1.0f, 0.0f, 10.0f, 0.001f, 0.1f));
	s->Add (new ToggleProperty (Property::WriteOnly, false));
	s->Add (new ToggleProperty (Property::WriteOnly, false));
	s->Add (new SignedProperty (DefaultLinearFlags, Note, 0, 131, 1, 1));
	s->Add (new SignedProperty (DefaultLinearFlags, 0, -6, 6, 1, 1));

	s->Add (new FloatProperty  (Property::WriteOnly, -1, -1, INT_MAX, 1, 1));
	s->Add (new SignedProperty (Property::WriteOnly, 0, 0, INT_MAX, 1, 1));
	s->Add (new SignedProperty (Property::WriteOnly, 0, 0, INT_MAX, 1, 1));
	s->Add (new SignedProperty (Property::WriteOnly, INT_MAX, 0, INT_MAX, 1, 1));
	s->Add (new ToggleProperty (Property::WriteOnly, true));

	return s;
}

void PoshSamplerPlugin::InitializeSampleDescription (SampleDesc* NewDesc, int num, int Note) {
       if (NewDesc) {
          NewDesc->Velocity = 1.0f;
          NewDesc->Pitch = 1.0f;
          NewDesc->TriggerUp = true;
          NewDesc->SampleRate = 44100;
          NewDesc->Stereo = false;
       }
}

void PoshSamplerPlugin::Describe(Spiral::Core::Description &d) {
     d.Value(DeviceVersion).Separator(" ");
     for (int n=0; n<NUM_SAMPLES; n++) {
         d.Value(SlotFloat (n, SLOT_VOLUME)).Separator(" ")
              .Value(SlotFloat (n, SLOT_PITCHMOD)).Separator(" ")
              .Value(SlotBool (n, SLOT_LOOP)).Separator(" ")
              .Value(SlotBool (n, SLOT_PINGPONG)).Separator(" ")
              .Value(SlotSigned (n, SLOT_NOTE)).Separator(" ")
              .Value(SlotSigned (n, SLOT_OCTAVE)).Separator(" ")
              .Value(SlotFloat (n, SLOT_SAMPLEPOS)).Separator(" ")
              .Value(SlotSigned (n, SLOT_PLAYSTART)).Separator(" ")
              .Value(SlotSigned (n, SLOT_LOOPSTART)).Separator(" ")
              .Value(SlotSigned (n, SLOT_LOOPEND)).Separator(" ")
              .Value(SlotSigned (n, SLOT_NOTE)).Separator(" ")
              .Value(SlotBool (n, SLOT_RETRIG)).Separator(" ");
     }
}

void PoshSamplerPlugin::Migrate(Spiral::Core::Description &d) {
     static const long SAMPLER_ID = 16;
     if (d.SavedAs() != SAMPLER_ID)
        return;

     const vector<string> v = d.Values();
     int version = v.empty() ? 0 : atoi(v[0].c_str());
     size_t perSample = (version > 1) ? 5 : 4;
     Description posh;
     if (v.size() == 1 + NUM_SAMPLES * perSample) {
        posh.Value(2);
        for (int n = 0; n < NUM_SAMPLES; n++) {
            const string *slot = &v[1 + n * perSample];
            string path = slot[perSample == 5 ? 3 : 2];
            const string &note = slot[perSample - 1];
            if (path == "None")
               path = "";
            posh.Value(slot[0])
                .Value(slot[1])
                .Value(0).Value(0)
                .Value(note)
                .Value(0)
                .Value(-1)
                .Value(0).Value(0).Value(0)
                .Value(note)
                .Value((int)path.size())
                .Value(path);
        }
     }
     d = posh;
}

void PoshSamplerPlugin::Apply(Spiral::Core::Description::Reader &r) {
     int version;
     r.Value(version);
     for (int n=0; n<NUM_SAMPLES; n++) {
         r.Value(SlotFloat (n, SLOT_VOLUME))
              .Value(SlotFloat (n, SLOT_PITCHMOD))
              .Value(SlotBool (n, SLOT_LOOP))
              .Value(SlotBool (n, SLOT_PINGPONG))
              .Value(SlotSigned (n, SLOT_NOTE))
              .Value(SlotSigned (n, SLOT_OCTAVE))
              .Value(SlotFloat (n, SLOT_SAMPLEPOS))
              .Value(SlotSigned (n, SLOT_PLAYSTART))
              .Value(SlotSigned (n, SLOT_LOOPSTART))
              .Value(SlotSigned (n, SLOT_LOOPEND))
              .Value(SlotSigned (n, SLOT_NOTE));
         if (version < 3) {
            int size;
            std::string Buf;
            r.Value(size);
            r.Value(Buf);
            if (size > 0 && Buf.size() == (size_t)size) {
               long loopEnd = SampleSigned (n, SLOT_LOOPEND);
               LoadSample (n, Buf);
               if (loopEnd > 0 && loopEnd < m_SampleVec[n]->GetLength())
                  SampleSigned (n, SLOT_LOOPEND) = loopEnd;
            }
         }
         if (version > 3) r.Value(SlotBool (n, SLOT_RETRIG));
         else SampleBool (n, SLOT_RETRIG) = true;
     }
}

void PoshSamplerPlugin::LoadSample (int n, const string &Name) {
     WavFile Wav;
     if (Wav.Open (Name, WavFile::READ)) {
        m_SampleVec[n]->Allocate (Wav.GetSize());
        Wav.Load (*m_SampleVec[n]);
        // andy preston - if this is not commented out, when we load a patch this overwrites the
        //                loaded data with the defaults
        // InitializeSampleDescription (m_SampleDescVec[n], n, n);
        m_SampleDescVec[n]->SampleRate = Wav.GetSamplerate();
        m_SampleDescVec[n]->Stereo = Wav.IsStereo();
        m_InitialPitch[n] = m_SampleDescVec[n]->Pitch;
        m_SampleDescVec[n]->Pitch = m_InitialPitch[n] * m_SampleDescVec[n]->SampleRate / (float)m_HostInfo->SAMPLERATE;
        SampleSigned (n, SLOT_LOOPEND) = m_SampleVec[n]->GetLength()-1;
     }
}

void PoshSamplerPlugin::SaveSample(int n, const string &Name)
{
	if (m_SampleVec[n]->GetLength()==0) return;

	WavFile Wav;
	Wav.Open(Name,WavFile::WRITE,WavFile::MONO);
	Wav.Save(*m_SampleVec[n]);
}

void PoshSamplerPlugin::Cut(int n, long s, long e)
{
	if (m_SampleVec[n]->GetLength()==0) return;
	m_SampleVec[n]->GetRegion(m_CopyBuffer, s, e);
	m_SampleVec[n]->Remove(s, e);
}

void PoshSamplerPlugin::Copy(int n, long s, long e)
{
	if (m_SampleVec[n]->GetLength()==0) return;
	m_SampleVec[n]->GetRegion(m_CopyBuffer, s, e);
}

void PoshSamplerPlugin::Paste(int n, long s, long e)
{
	if (m_SampleVec[n]->GetLength()==0) return;
	m_SampleVec[n]->Insert(m_CopyBuffer, s);
}

void PoshSamplerPlugin::Mix(int n, long s, long e)
{
	if (m_SampleVec[n]->GetLength()==0) return;
	m_SampleVec[n]->Mix(m_CopyBuffer, s);
}

void PoshSamplerPlugin::Crop(int n, long s, long e)
{
	if (m_SampleVec[n]->GetLength()==0) return;
	m_SampleVec[n]->Remove(0, s);
	m_SampleVec[n]->Remove(e, m_SampleVec[n]->GetLength()-1);
}

void PoshSamplerPlugin::Reverse(int n, long s, long e)
{
	if (m_SampleVec[n]->GetLength()==0) return;
	m_SampleVec[n]->Reverse(s, e);
}

void PoshSamplerPlugin::Amp(int n, long s, long e)
{
	if (m_SampleVec[n]->GetLength()==0) return;
	for (int m=0; m<m_SampleVec[n]->GetLength(); m++)
	{
		m_SampleVec[n]->Set(m,(*m_SampleVec[n])[m]*SampleFloat (n, SLOT_VOLUME));
	}
}

// The sprintf's in here should use strstream, but the current implementation (GCC 2.95.3) is buggy

bool PoshSamplerPlugin::SaveExternalFiles (const string &Dir) {
     char temp[256];
     for (int n=0; n<NUM_SAMPLES; n++) {
         // if it's not empty
         if (m_SampleVec[n]->GetLength() != 0) {
            sprintf (temp, "%sPoshSampler%d_%d.wav", Dir.c_str(), GetID(), n);
            SaveSample (n, temp);
         }
     }
     return true;
}

void PoshSamplerPlugin::LoadExternalFiles(const string &Dir, int withID) {
     int UseID = (withID==-1) ? GetID() : withID;
     char temp[256];
     for (int n=0; n<NUM_SAMPLES;  n++) {
         sprintf (temp, "%sPoshSampler%d_%d.wav", Dir.c_str(), UseID, n);
         LoadSample (n, temp);
     }
}
