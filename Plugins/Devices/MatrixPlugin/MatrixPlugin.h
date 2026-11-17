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

#include "Device.h"
#include "EventVoices.h"
#include "NoteTable.h"
#include <vector>

#ifndef MatrixPLUGIN
#define MatrixPLUGIN

static const int MATX         = 64;
static const int MATY         = 32;
static const int NUM_PATTERNS = 16;
static const int NUM_PATSEQ   = 16;
static const int NUM_TRIGGERS = 16;
static const int MATRIX_PITCHES = sizeof(NoteTable) / sizeof(NoteTable[0]);

struct Pattern
{
	int Length;
	float Speed;
	int Octave;
	int Triggers;
	float Volume[MATX][MATRIX_PITCHES];
	bool Matrix[MATX][MATRIX_PITCHES];
	unsigned char End[MATX][MATRIX_PITCHES];
};

class MatrixPatternProperty : public Spiral::Core::VectorProperty
{
public:
	MatrixPatternProperty(Spiral::Core::SignedType Flags): VectorProperty(Flags) {}
	MatrixPatternProperty(const MatrixPatternProperty &Copy): VectorProperty(Copy) {}
	virtual Spiral::Core::Property *Duplicate() { return new MatrixPatternProperty(*this); }
	virtual std::string ClassName() { return "MatrixPattern"; }
};

class MatrixPlugin : public Spiral::Core::Device
{
public:
 	MatrixPlugin();
	virtual ~MatrixPlugin();
	
	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void Reset();
	virtual void SetVoice(SpiralPlugin *first, unsigned voice, unsigned voices);
	virtual void ExecuteCommands();
	virtual void Describe(Spiral::Core::Description &d);
	virtual void Apply(Spiral::Core::Description::Reader &r);
		
	bool  GetNoteCut()               { return AsBoolean(m_NoteCut); }
	bool  GetTriggerSelect()         { return AsBoolean(m_TriggerSelect); }
	int   GetCurrent()               { return AsSigned(m_Current); }
	float GetStepTime()              { return m_StepTime; }
	Pattern* GetPattern() 			 { return &m_Matrix[AsSigned(m_Current)]; }
	int   GetPatSeq(int n)           { return ((Spiral::Core::SignedProperty *)m_PatSeq[n])->Value.AsSigned; }
	Spiral::Core::SignedProperty &PatSeq(int n) { return *(Spiral::Core::SignedProperty *)m_PatSeq[n]; }
	void  SetPatSeq(int n, int Value){ ((Spiral::Core::SignedProperty *)m_PatSeq[n])->Value.AsSigned = Value; }
		
	enum GUICommands{NONE,MAT_LENGTH,MAT_SPEED,MAT_ACTIVATE,MAT_DEACTIVATE,
					 MAT_OCTAVE,COPY,PASTE,CLEAR,TUP,TDOWN,MAT_VOLUME,
					 SET_CURRENT,SET_PATSEQ,MAT_END,MAT_TRIGGERS};
	
	struct GUIArgs
	{
		int Num;
		int Length;
		float Speed;
		int X,Y;
		int Octave;
		float Volume;
		int End;
	};
	
private:

	GUIArgs m_GUIArgs;

    void PastePattern();
    void ClearPattern();
    void TransposeUp();
    bool CanTransposeUp();
    void TransposeDown();
    bool CanTransposeDown();
	
	float m_TickTime;
	int Shown(int x, int y) const;
	void RefreshEcho();
	bool EchoStale();
	int   m_Echoed;
	// The notes of the pattern being edited, for the panel: a row to a
	// note, as its start step, the step it ends before, its pitch, its
	// volume, and whether the voices cannot play all of it (never, until
	// this device has voices). Only the rows in use are handed over.
	enum { NOTE_CELLS = 5 };
	float m_EchoNotes[MATX*MATRIX_PITCHES][NOTE_CELLS];
	Spiral::Core::UnsignedProperty m_NoteCount;
	int   m_EchoPatSeq[NUM_PATSEQ];
	float m_StepTime;
	Spiral::Core::FloatProperty  m_Time;
	Spiral::Core::SignedProperty m_Step;
	Spiral::Core::ToggleProperty m_Loop;
	Spiral::Core::ToggleProperty m_NoteCut;
	Spiral::Core::ToggleProperty m_TriggerSelect;
	
	Spiral::Core::SignedProperty m_Current;
	Spiral::Core::SignedProperty m_Pattern;
	int Edited() const
	{
		const int n = m_Pattern.Value.AsSigned;
		return n >= 0 && n < NUM_PATTERNS ? n : 0;
	}
	Spiral::Core::SignedProperty m_Length;
	Spiral::Core::FloatProperty  m_Speed;
	Spiral::Core::SignedProperty m_Octave;
	Spiral::Core::SignedProperty m_TriggerRow;
	Pattern m_Matrix[NUM_PATTERNS];
	float m_TriggerLevel[MATY];
	Spiral::Core::ListProperty m_PatSeq;

	Spiral::Core::ListProperty m_Patterns;
	enum { PatternFields = 4 };
	Spiral::Core::VectorProperty &PatternProperties(int n) { return *(Spiral::Core::VectorProperty *)m_Patterns[n]; }
	Spiral::Core::UnsignedProperty &PatternOctave(int n) { return *(Spiral::Core::UnsignedProperty *)PatternProperties(n)[0]; }
	Spiral::Core::UnsignedProperty &PatternLength(int n) { return *(Spiral::Core::UnsignedProperty *)PatternProperties(n)[1]; }
	Spiral::Core::FloatProperty    &PatternSpeed(int n)  { return *(Spiral::Core::FloatProperty *)PatternProperties(n)[2]; }
	Spiral::Core::UnsignedProperty &PatternTriggers(int n) { return *(Spiral::Core::UnsignedProperty *)PatternProperties(n)[3]; }
	int NoteCount(int n) { return (int)PatternProperties(n).size() - PatternFields; }
	Spiral::Core::NoteProperty &Note(int n, int k) { return *(Spiral::Core::NoteProperty *)PatternProperties(n)[PatternFields + k]; }
	static int NotePitch(const Spiral::Core::NoteProperty &note)
		{ return (int)note.Octave * 12 + (int)note.Note - 1; }
	static void SetPitch(Spiral::Core::NoteProperty &note, int pitch);
	void PatternsFromGrid();
	void GridFromPatterns();
	void ApplyNamed(Spiral::Core::Description::Reader &r, int version);
	void AddNote(int pattern, int x, int pitch, float volume, int end);
	static int NoteEnd(const Pattern &p, int x, int pitch);
	static int Covering(const Pattern &p, int step, int pitch);
	void ClearNotes(int pattern);
	int   m_CurPatSeq;
	
	float m_CurrentNoteCV;
	float m_CurrentTriggerCV;
	
	bool  m_Triggered;
	bool  m_ClockHigh;
	Pattern m_Clipboard;
	bool m_Copied;
	bool  m_PatAdvance;
	bool  m_PatReset;

	void NoteEvents(const Pattern &p, int step, unsigned frame);
	enum { AnyVoice = -1, NoVoice = -2 };
	int VoiceFor(int key);
	void Send(const Spiral::Core::Event &event, unsigned frame, int lane = -1);
	MatrixPlugin *m_First;
	unsigned m_Voice, m_VoiceCount;
	int m_RowID[MATRIX_PITCHES];
	int m_RowStart[MATRIX_PITCHES];
	int m_NextID;
	bool m_ResetEvents;
	Spiral::Core::EventVoices m_Voices;
	std::vector<float> m_Pitch, m_Gate;
};

#endif
