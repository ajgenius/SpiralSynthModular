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
#include "Finite.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include "SpiralIcon.xpm"
#include "NoteTable.h"
#include "RiffWav.h"
#include <stdio.h>
#include <stdlib.h>
#include <sstream>

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

}

///////////////////////////////////////////////////////

PoshSamplerPlugin::PoshSamplerPlugin() :
m_Recording(false),
m_LastRecording(false)
{
        for (int n=0; n<NUM_SAMPLES; n++) m_InitialPitch[n] = 1.0f;
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
        {
            static const char *const names[SLOT_COUNT] = {
                "VOLUME", "PITCHMOD", "LOOP", "PINGPONG", "NOTE", "OCTAVE",
                "SAMPLEPOS", "PLAYSTART", "LOOPSTART", "LOOPEND", "RETRIG" };
            for (int n=0; n<NUM_SAMPLES; n++)
                for (int s=0; s<SLOT_COUNT; s++) {
                    char key[24], hint[40];
                    snprintf (key, sizeof key, "%s%d", names[s], n);
                    snprintf (hint, sizeof hint, "Sample %d %s", n + 1, names[s]);
                    RegisterSharedProperty (*Slot (n, (SampleSlot)s), key, hint);
                }
        }
        m_CurrentSample = UnsignedProperty (DefaultLinearFlags, 0, 0, NUM_SAMPLES - 1, 1, 1);
        RegisterSharedProperty (m_CurrentSample, "CurrentSample", "The sample shown");
        m_RecordingProp = ToggleProperty (Property::ReadOnly, false);
        RegisterSharedProperty (m_RecordingProp, "Recording", "Recording into the sample");

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
        m_AudioCH->Register ("Bool", &m_GUIArgs.Boole);
        m_AudioCH->Register ("Start", &m_GUIArgs.Start);
        m_AudioCH->Register ("End", &m_GUIArgs.End);
        m_AudioCH->Register ("LoopStart", &m_GUIArgs.LoopStart);
        m_AudioCH->RegisterData ("Name", ChannelHandler::INPUT, &m_GUIArgs.Name, sizeof (m_GUIArgs.Name));
        m_AudioCH->RegisterData ("SampleBuffer", ChannelHandler::OUTPUT_REQUEST, &m_SampleBuffer, TRANSBUF_SIZE);
        m_AudioCH->Register ("SampleSize", &m_SampleSize, ChannelHandler::OUTPUT_REQUEST);
        {
        	std::vector<std::string> none, args;
        	args.clear(); args.push_back("Num"); args.push_back("Start"); args.push_back("End");
        	m_AudioCH->RegisterCommand("AMP", AMP, args, "iii");
        	args.clear(); args.push_back("Num"); args.push_back("Start"); args.push_back("End");
        	m_AudioCH->RegisterCommand("COPY", COPY, args, "iii");
        	args.clear(); args.push_back("Num"); args.push_back("Start"); args.push_back("End");
        	m_AudioCH->RegisterCommand("CROP", CROP, args, "iii");
        	args.clear(); args.push_back("Num"); args.push_back("Start"); args.push_back("End");
        	m_AudioCH->RegisterCommand("CUT", CUT, args, "iii");
        	m_AudioCH->RegisterCommand("GETSAMPLE", GETSAMPLE, none);
        	args.clear(); args.push_back("Num"); args.push_back("Name");
        	m_AudioCH->RegisterCommand("LOAD", LOAD, args, "is");
        	args.clear(); args.push_back("Num"); args.push_back("Start"); args.push_back("End");
        	m_AudioCH->RegisterCommand("MIX", MIX, args, "iii");
        	args.clear(); args.push_back("Num"); args.push_back("Start"); args.push_back("End");
        	m_AudioCH->RegisterCommand("PASTE", PASTE, args, "iii");
        	args.clear(); args.push_back("Num"); args.push_back("Start"); args.push_back("End");
        	m_AudioCH->RegisterCommand("REV", REV, args, "iii");
        	args.clear(); args.push_back("Num"); args.push_back("Name");
        	m_AudioCH->RegisterCommand("SAVE", SAVE, args, "is");
        	args.clear(); args.push_back("Num");
        	m_AudioCH->RegisterCommand("SETCURRENT", SETCURRENT, args, "i");
        	args.clear(); args.push_back("Num"); args.push_back("Start"); args.push_back("End"); args.push_back("LoopStart");
        	m_AudioCH->RegisterCommand("SETPLAYPOINTS", SETPLAYPOINTS, args, "iiii");
        	args.clear(); args.push_back("Bool");
        	m_AudioCH->RegisterCommand("SETREC", SETREC, args, "i");
        	args.clear(); args.push_back("Num"); args.push_back("Name");
        	m_AudioCH->RegisterCommand("DRAW", DRAW, args, "is");
        }
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
         const long length = m_SampleVec[s]->GetLength();
         if (length > 0 && (SampleSigned (s, SLOT_LOOPEND) <= 0 || SampleSigned (s, SLOT_LOOPEND) >= length))
            SampleSigned (s, SLOT_LOOPEND) = length - 1;
     }
}

void PoshSamplerPlugin::Execute() {
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
                   const int note = SampleSigned (s, SLOT_NOTE);
                   if (GetInput (s*2 + 1, n) > 0 || (note >= 0 && note < 132 && feq (Freq, NoteTable[note], 0.01f))) {
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
                      if (SampleBool (s, SLOT_PINGPONG)) S->Pong = true;
                      else SampleFloat (s, SLOT_SAMPLEPOS) = SampleSigned (s, SLOT_LOOPSTART);
                   }
                   else {
                      SampleFloat (s, SLOT_SAMPLEPOS) = -1;
                   }
                }
                // if the sample has ended ponging
                if (S->Pong && SampleFloat (s, SLOT_SAMPLEPOS) <= SampleSigned (s, SLOT_LOOPSTART))
                   S->Pong = false;
                float &position = SampleFloat (s, SLOT_SAMPLEPOS);
                if (position != -1) {
                   if (!Spiral::Core::IsFinite (position)) position = -1;
                   else if (position < 0) position = 0;
                   else if (position > m_SampleVec[s]->GetLength() - 1) position = m_SampleVec[s]->GetLength() - 1;
                }
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
                   Freq = ldexpf (Freq, SampleSigned (s, SLOT_OCTAVE));
                   if (S->Pong) SampleFloat (s, SLOT_SAMPLEPOS) -= Freq * SampleFloat (s, SLOT_PITCHMOD);
                   else SampleFloat (s, SLOT_SAMPLEPOS) += Freq * SampleFloat (s, SLOT_PITCHMOD);
                }
             }
         }
     }
     // record
     if (m_Recording && InputExists (REC_INPUT)) {
        int s = 0; //GUI->GetCurrentSample();
        if (!m_LastRecording) m_SampleVec[s]->Clear();
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
     m_LastRecording = m_Recording;
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
          case DRAW:
             Draw (m_GUIArgs.Num, m_GUIArgs.Name);
             break;
          case SETCURRENT:
             m_Current = m_GUIArgs.Num;
             AsUnsigned (m_CurrentSample) = m_Current;
             break;
          case GETSAMPLE:
             m_AudioCH->SetupBulkTransfer ((void*)m_SampleVec[m_Current]->GetBuffer());
             m_SampleSize = m_SampleVec[m_Current]->GetLength();
             break;
        }
     }
     m_Current = AsUnsigned (m_CurrentSample);
     AsBoolean (m_RecordingProp) = m_Recording;
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

	s->Add (new FloatProperty  (Property::ReadOnly, -1, -1, INT_MAX, 1, 1));
	s->Add (new SignedProperty (Property::ReadOnly, 0, 0, INT_MAX, 1, 1));
	s->Add (new SignedProperty (Property::ReadOnly, 0, 0, INT_MAX, 1, 1));
	s->Add (new SignedProperty (Property::ReadOnly, INT_MAX, 0, INT_MAX, 1, 1));
	s->Add (new ToggleProperty (Property::WriteOnly, true));

	return s;
}

void PoshSamplerPlugin::InitializeSampleDescription (SampleDesc* NewDesc, int num, int Note) {
       if (NewDesc) {
          NewDesc->Velocity = 1.0f;
          NewDesc->Pitch = 1.0f;
          NewDesc->TriggerUp = true;
          NewDesc->Pong = false;
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
         SampleSigned (n, SLOT_NOTE) = std::max(0L, std::min(131L, (long)SampleSigned (n, SLOT_NOTE)));
         if (version < 3 && !r.Named()) {
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
        m_InitialPitch[n] = 1.0f;
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

// A stroke drawn on the waveform: each sample's place and its new value, in
// pairs. Nothing is set unless every pair is a place in the sample and a value
// within one of zero.
void PoshSamplerPlugin::Draw(int n, const char *stroke)
{
	if (n<0 || n>=NUM_SAMPLES) return;
	std::istringstream in(stroke);
	std::vector<std::pair<long, float> > points;
	long place;
	float value;
	while (in >> place)
	{
		if (!(in >> value) || place<0 || place>=m_SampleVec[n]->GetLength() || !(value>=-1 && value<=1)) return;
		points.push_back(std::make_pair(place, value));
	}
	if (!in.eof()) return;
	for (size_t i=0; i<points.size(); i++) m_SampleVec[n]->Set(points[i].first, points[i].second);
}

void PoshSamplerPlugin::Amp(int n, long s, long e)
{
	if (m_SampleVec[n]->GetLength()==0) return;
	for (int m=0; m<m_SampleVec[n]->GetLength(); m++)
	{
		m_SampleVec[n]->Set(m,(*m_SampleVec[n])[m]*SampleFloat (n, SLOT_VOLUME));
	}
}

bool PoshSamplerPlugin::SaveExternalFiles (const string &Dir) {
     for (int n=0; n<NUM_SAMPLES; n++) {
         // if it's not empty
         if (m_SampleVec[n]->GetLength() != 0) {
            ostringstream temp;
            temp << Dir << "PoshSampler" << GetID() << "_" << n << ".wav";
            SaveSample (n, temp.str());
         }
     }
     return true;
}

void PoshSamplerPlugin::LoadExternalFiles(const string &Dir, int withID) {
     int UseID = (withID==-1) ? GetID() : withID;
     for (int n=0; n<NUM_SAMPLES;  n++) {
         ostringstream temp;
         temp << Dir << "PoshSampler" << UseID << "_" << n << ".wav";
         const long loopEnd = SampleSigned (n, SLOT_LOOPEND);
         LoadSample (n, temp.str());
         if (loopEnd > 0 && loopEnd < m_SampleVec[n]->GetLength())
            SampleSigned (n, SLOT_LOOPEND) = loopEnd;
     }
}
