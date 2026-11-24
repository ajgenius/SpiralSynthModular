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

#ifndef PoshSamplerPLUGIN
#define PoshSamplerPLUGIN

#include "SamplerVoices.h"

#include "Device.h"
#include "VoicePool.h"

static const int NUM_SAMPLES = SamplerVoices::Slots;
static const int TRANSBUF_SIZE = 0x10000;

struct SampleDesc {
       float Velocity, Pitch;
       bool TriggerUp, Pong;
       int SampleRate;
       bool Stereo;
};

// The sampler runs once, whatever voices its patch has, as ajgenius set on
// 10 November 2026. Each sample has its one trigger and its one playhead; the
// voices are on the mixed output, where each sample that is playing has one
// while there are enough. A new attack steals the oldest voice. Individual outputs are mono.
class PoshSamplerPlugin : public Spiral::Core::Device, public VoiceOutputSource {
   public:
      PoshSamplerPlugin();
      virtual ~PoshSamplerPlugin();
      virtual PluginInfo &Initialise (const HostInfo *Host);
      virtual void Execute();
      virtual void Reset();
      virtual void ExecuteCommands();
      virtual void SetVoice(SpiralPlugin *first, unsigned voice, unsigned voices);
      virtual Sample *VoiceOutput(unsigned port, unsigned voice);
      virtual bool SumsVoices(unsigned port) const { return port == 0; }
      virtual void Describe(Spiral::Core::Description &d);
      virtual void Apply(Spiral::Core::Description::Reader &r);
      virtual void Migrate(Spiral::Core::Description &d);
      virtual bool SaveExternalFiles (const std::string &Dir);
      virtual void LoadExternalFiles (const std::string &Dir, int withID=-1);
      enum GUICommands {NONE, LOAD, SAVE, SETPLAYPOINTS, SETREC, CUT, COPY, PASTE, CROP, MIX,
                        REV, AMP, SETCURRENT, GETSAMPLE, DRAW};
      struct GUIArgs {
         int Num;
         bool Boole;
         long Start, End, LoopStart;
         char Name[256];
      };
      Spiral::Core::UnsignedProperty m_CurrentSample;
      Spiral::Core::ToggleProperty m_RecordingProp;
      void LoadSample (int n, const std::string &Name);
      void SaveSample (int n, const std::string &Name);
      Sample* GetSample (int n) { return m_SampleVec[n]; }
      float GetVolume (int n) { return SampleFloat (n, SLOT_VOLUME); }
      float GetPitch (int n) { return SampleFloat (n, SLOT_PITCHMOD); }
      bool GetReTrig (int n) { return SampleBool (n, SLOT_RETRIG); }
      bool GetLoop (int n) { return SampleBool (n, SLOT_LOOP); }
      bool GetPingPong (int n) { return SampleBool (n, SLOT_PINGPONG); }
      int GetNote (int n) { return SampleSigned (n, SLOT_NOTE); }
      int GetOctave (int n) { return SampleSigned (n, SLOT_OCTAVE)+6; }
      long GetPlayStart (int n) { return SampleSigned (n, SLOT_PLAYSTART); }
      long GetLoopStart (int n) { return SampleSigned (n, SLOT_LOOPSTART); }
      long GetLoopEnd (int n) { return SampleSigned (n, SLOT_LOOPEND); }
      std::vector<Sample*> m_SampleVec;
      std::vector<SampleDesc*> m_SampleDescVec;

      enum SampleSlot
      {
         SLOT_VOLUME,
         SLOT_PITCHMOD,
         SLOT_LOOP,
         SLOT_PINGPONG,
         SLOT_NOTE,
         SLOT_OCTAVE,
         SLOT_SAMPLEPOS,
         SLOT_PLAYSTART,
         SLOT_LOOPSTART,
         SLOT_LOOPEND,
         SLOT_RETRIG,
         SLOT_COUNT
      };

      Spiral::Core::VectorProperty m_Samples;

      Spiral::Core::VectorProperty *NewSampleSettings (int Note);

      Spiral::Core::Property *Slot (int n, SampleSlot s)
      {
         return (*static_cast<Spiral::Core::VectorProperty *>(m_Samples[n]))[s];
      }

      Spiral::Core::FloatType &SampleFloat (int n, SampleSlot s)
      { return ((Spiral::Core::FloatProperty *)Slot (n, s))->Value.AsFloat; }

      Spiral::Core::SignedType &SampleSigned (int n, SampleSlot s)
      { return ((Spiral::Core::SignedProperty *)Slot (n, s))->Value.AsSigned; }

      bool &SampleBool (int n, SampleSlot s)
      { return ((Spiral::Core::ToggleProperty *)Slot (n, s))->Value.AsBoolean; }

      Spiral::Core::FloatProperty &SlotFloat (int n, SampleSlot s)
      { return *(Spiral::Core::FloatProperty *)Slot (n, s); }

      Spiral::Core::SignedProperty &SlotSigned (int n, SampleSlot s)
      { return *(Spiral::Core::SignedProperty *)Slot (n, s); }

      Spiral::Core::ToggleProperty &SlotBool (int n, SampleSlot s)
      { return *(Spiral::Core::ToggleProperty *)Slot (n, s); }
   private:
      long TriggerStart(int slot, int frame);
      unsigned m_PatchVoices;
      SamplerVoices m_Voices;
      std::vector<Sample> m_MixedVoices;
      Sample m_Silence;
      void SizeVoices();
      // An older save had a whole sample's end one frame short.
      bool m_OlderSave;
      void RestoreSample(int slot, const std::string &file);
      void InitializeSampleDescription (SampleDesc* NewDesc, int num, int Note);
      void Cut (int n, long s, long e);
      void Copy (int n, long s, long e);
      void Paste (int n, long s, long e);
      void Mix (int n, long s, long e);
      void Crop (int n, long s, long e);
      void Reverse (int n, long s, long e);
      void Amp (int n, long s, long e);
      void Draw (int n, const char *stroke);
      int m_Current;
      GUIArgs m_GUIArgs;
      Sample m_CopyBuffer;
      bool m_Recording, m_LastRecording;
      char m_SampleBuffer[TRANSBUF_SIZE];
      long m_SampleSize;
      float m_InitialPitch[NUM_SAMPLES];
};

#endif
