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

#include "../SpiralPlugin.h"
#include <FL/Fl_Pixmap.H>

static const int NUM_SAMPLES = 8;
static const int TRANSBUF_SIZE = 0x10000;

struct SampleDesc {
       std::string Pathname;
       float Volume, Velocity, Pitch, PitchMod;
       bool Loop, PingPong;
       int Note, Octave;
       bool TriggerUp;
       float SamplePos;
       int SampleRate;
       bool Stereo;
       long PlayStart, LoopStart, LoopEnd;
       bool ReTrig;
};

class PoshSamplerPlugin : public SpiralPlugin {
   public:
      PoshSamplerPlugin();
      virtual ~PoshSamplerPlugin();
      virtual PluginInfo &Initialise (const HostInfo *Host);
      virtual SpiralGUIType *CreateGUI();
      virtual void Execute();
      virtual void Reset();
      virtual void ExecuteCommands();
      virtual void StreamOut (std::ostream &s);
      virtual void StreamIn (std::istream &s);
      virtual bool SaveExternalFiles (const std::string &Dir);
      virtual void LoadExternalFiles (const std::string &Dir, int withID=-1);
      enum GUICommands {NONE, LOAD, SAVE, SETVOL, SETPITCH, SETLOOP, SETPING, SETNOTE, SETOCT,
                        SETPLAYPOINTS, SETREC, CUT, COPY, PASTE, CROP, MIX, REV, AMP, SETCURRENT,
	                GETSAMPLE, SETRETRIG};
      struct GUIArgs {
         int Num;
         float Value;
         bool Boole;
         int Int;
         long Start, End, LoopStart;
         char Name[256];
      };
      void LoadSample (int n, const std::string &Name);
      void SaveSample (int n, const std::string &Name);
      Sample* GetSample (int n) { return m_SampleVec[n]; }
      float GetVolume (int n) { return m_SampleDescVec[n]->Volume; }
      float GetPitch (int n) { return m_SampleDescVec[n]->PitchMod; }
      bool GetReTrig (int n) { return m_SampleDescVec[n]->ReTrig; }
      bool GetLoop (int n) { return m_SampleDescVec[n]->Loop; }
      bool GetPingPong (int n) { return m_SampleDescVec[n]->PingPong; }
      int GetNote (int n) { return m_SampleDescVec[n]->Note; }
      int GetOctave (int n) { return m_SampleDescVec[n]->Octave+6; }
      long GetPlayStart (int n) { return m_SampleDescVec[n]->PlayStart; }
      long GetLoopStart (int n) { return m_SampleDescVec[n]->LoopStart; }
      long GetLoopEnd (int n) { return m_SampleDescVec[n]->LoopEnd; }
      std::vector<Sample*> m_SampleVec;
      std::vector<SampleDesc*> m_SampleDescVec;
   private:
      void Cut (int n, long s, long e);
      void Copy (int n, long s, long e);
      void Paste (int n, long s, long e);
      void Mix (int n, long s, long e);
      void Crop (int n, long s, long e);
      void Reverse (int n, long s, long e);
      void Amp (int n, long s, long e);
      int m_Current;
      GUIArgs m_GUIArgs;
      Sample m_CopyBuffer;
      bool m_Recording;
      char m_SampleBuffer[TRANSBUF_SIZE];
      long m_SampleSize, m_CurrentPlayPos;
      float m_InitialPitch[NUM_SAMPLES];
};

#endif
