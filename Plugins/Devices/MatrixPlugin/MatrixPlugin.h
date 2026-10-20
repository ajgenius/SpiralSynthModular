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

#ifndef MatrixPLUGIN
#define MatrixPLUGIN

static const int MATX         = 64;
static const int MATY         = 32;
static const int NUM_PATTERNS = 16;
static const int NUM_PATSEQ   = 16;

struct Pattern
{
	int Length;
	float Speed;
	int Octave;
	float Volume[MATX][MATY];
	bool Matrix[MATX][MATY];
};

class MatrixPlugin : public Spiral::Core::Device
{
public:
 	MatrixPlugin();
	virtual ~MatrixPlugin();
	
	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void Reset();
	virtual void ExecuteCommands();
	virtual void Describe(Spiral::Core::Description &d);
	virtual void Apply(Spiral::Core::Description::Reader &r);
		
	bool  GetNoteCut()               { return AsBoolean(m_NoteCut); }
	int   GetCurrent()               { return AsSigned(m_Current); }
	float GetStepTime()              { return m_StepTime; }
	Pattern* GetPattern() 			 { return &m_Matrix[AsSigned(m_Current)]; }
	int   GetPatSeq(int n)           { return ((Spiral::Core::SignedProperty *)m_PatSeq[n])->Value.AsSigned; }
	Spiral::Core::SignedProperty &PatSeq(int n) { return *(Spiral::Core::SignedProperty *)m_PatSeq[n]; }
	void  SetPatSeq(int n, int Value){ ((Spiral::Core::SignedProperty *)m_PatSeq[n])->Value.AsSigned = Value; }
		
	enum GUICommands{NONE,MAT_LENGTH,MAT_SPEED,MAT_ACTIVATE,MAT_DEACTIVATE,
					 MAT_OCTAVE,COPY,PASTE,CLEAR,TUP,TDOWN,MAT_VOLUME,
					 SET_CURRENT,SET_PATSEQ};
	
	struct GUIArgs
	{
		int Num;
		int Length;
		float Speed;
		int X,Y;
		int Octave;
		float Volume;
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
	float m_StepTime;
	Spiral::Core::FloatProperty  m_Time;
	Spiral::Core::SignedProperty m_Step;
	Spiral::Core::ToggleProperty m_Loop;
	Spiral::Core::ToggleProperty m_NoteCut;
	
	Spiral::Core::SignedProperty m_Current;
	int   m_GUICurrent;
	Pattern m_Matrix[NUM_PATTERNS];
	float m_TriggerLevel[MATY];
	Spiral::Core::VectorProperty m_PatSeq;

	Spiral::Core::VectorProperty m_Patterns;
	Spiral::Core::VectorProperty &PatternProperties(int n) { return *(Spiral::Core::VectorProperty *)m_Patterns[n]; }
	Spiral::Core::SignedProperty &PatternLength(int n) { return *(Spiral::Core::SignedProperty *)PatternProperties(n)[0]; }
	Spiral::Core::FloatProperty  &PatternSpeed(int n)  { return *(Spiral::Core::FloatProperty *)PatternProperties(n)[1]; }
	Spiral::Core::SignedProperty &PatternOctave(int n) { return *(Spiral::Core::SignedProperty *)PatternProperties(n)[2]; }
	Spiral::Core::VectorProperty &Notes(int n) { return *(Spiral::Core::VectorProperty *)PatternProperties(n)[3]; }
	Spiral::Core::SignedProperty &NoteX(int n, int k) { return *(Spiral::Core::SignedProperty *)(*(Spiral::Core::VectorProperty *)Notes(n)[k])[0]; }
	Spiral::Core::SignedProperty &NoteY(int n, int k) { return *(Spiral::Core::SignedProperty *)(*(Spiral::Core::VectorProperty *)Notes(n)[k])[1]; }
	Spiral::Core::FloatProperty  &NoteVolume(int n, int k) { return *(Spiral::Core::FloatProperty *)(*(Spiral::Core::VectorProperty *)Notes(n)[k])[2]; }
	void PatternsFromGrid();
	void GridFromPatterns();
	void AddNote(int pattern, int x, int y, float volume);
	void ClearNotes(int pattern);
	int   m_CurPatSeq;
	
	float m_CurrentNoteCV;
	float m_CurrentTriggerCV;
	
	bool  m_Triggered;
	bool  m_ClockHigh;
	int m_CopyPattern;
	bool  m_PatAdvance;
	bool  m_PatReset;
};

#endif
