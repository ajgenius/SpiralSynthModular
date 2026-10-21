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
#include <cstring>
#include "MatrixPlugin.h"
#include <algorithm>
#include "SpiralIcon.xpm"
#include "NoteTable.h"

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

SpiralPlugin* SpiralPlugin_CreateInstance()
{
	return new MatrixPlugin;
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
	return 0x0012;
}

string SpiralPlugin_GetName()
{
	return "Matrix";
}

string SpiralPlugin_GetGroupName()
{
	return "Sequencing";
}
}

///////////////////////////////////////////////////////

// A pattern's or a sequence step's name, as the private tree gives them.
static std::string Numbered(int n)
{
	char text[16];
	snprintf(text, sizeof text, "%d", n);
	return text;
}

MatrixPlugin::MatrixPlugin() :
m_TickTime(1.0f),
m_StepTime(1.0f),
m_CurPatSeq(0),
m_CurrentNoteCV(0),
m_CurrentTriggerCV(0),
m_Triggered(false),
m_ClockHigh(false),
m_CopyPattern(0),
m_PatAdvance(false),
m_PatReset(false)
{
	// Named as the private tree names them: each step of the sequence and
	// each pattern by its number, and a pattern's octave, length and speed.
	m_PatSeq = ListProperty(Property::WriteOnly);
	for (int n=0; n<NUM_PATSEQ; n++)
	{
		SignedProperty *step = new SignedProperty(DefaultLinearFlags, -1, -1, NUM_PATTERNS - 1, 1, 1);
		step->SetLabel(Numbered(n));
		m_PatSeq.Add(step);
	}

	m_Patterns = ListProperty(Property::WriteOnly);
	for (int n=0; n<NUM_PATTERNS; n++)
	{
		MatrixPatternProperty *pattern = new MatrixPatternProperty(Property::WriteOnly);
		pattern->SetLabel(Numbered(n));
		UnsignedProperty *octave = new UnsignedProperty(DefaultLinearFlags, 0, 0, 6, 1, 1);
		octave->SetLabel("Octave");
		UnsignedProperty *length = new UnsignedProperty(DefaultLinearFlags, 64, 1, MATX, 1, 1);
		length->SetLabel("Length");
		FloatProperty *speed = new FloatProperty(DefaultLinearFlags, 1.0f, 0.0f, 16.0f, 0.001f, 0.1f);
		speed->SetLabel("Speed");
		UnsignedProperty *triggers = new UnsignedProperty(DefaultLinearFlags, 0, 0, MATRIX_PITCHES - NUM_TRIGGERS, 1, 1);
		triggers->SetLabel("Triggers");
		pattern->Add(octave);
		pattern->Add(length);
		pattern->Add(speed);
		pattern->Add(triggers);
		m_Patterns.Add(pattern);
	}
	RegisterSharedProperty(m_Patterns, "Patterns", "Patterns");

	m_Version=4;

	m_PluginInfo.Name="Matrix";
	m_PluginInfo.Width=600;
	m_PluginInfo.Height=270;
	m_PluginInfo.NumInputs=5;
	m_PluginInfo.NumOutputs=19;
	m_PluginInfo.PortTips.push_back("Play Trigger");	
	m_PluginInfo.PortTips.push_back("StepTime CV");	
	m_PluginInfo.PortTips.push_back("Input Pitch CV");	
	m_PluginInfo.PortTips.push_back("Input Trigger CV");
	m_PluginInfo.PortTips.push_back("External Clock");
	//m_PluginInfo.PortTips.push_back("Pattern Advance");
	//m_PluginInfo.PortTips.push_back("Pattern Reset");
	m_PluginInfo.PortTips.push_back("Output Pitch");	
	m_PluginInfo.PortTips.push_back("Output Trigger");
	m_PluginInfo.PortTips.push_back("Trigger 1");
	m_PluginInfo.PortTips.push_back("Trigger 2");
	m_PluginInfo.PortTips.push_back("Trigger 3");
	m_PluginInfo.PortTips.push_back("Trigger 4");
	m_PluginInfo.PortTips.push_back("Trigger 5");
	m_PluginInfo.PortTips.push_back("Trigger 6");
	m_PluginInfo.PortTips.push_back("Trigger 7");
	m_PluginInfo.PortTips.push_back("Trigger 8");
	m_PluginInfo.PortTips.push_back("Trigger 9");
	m_PluginInfo.PortTips.push_back("Trigger 10");
	m_PluginInfo.PortTips.push_back("Trigger 11");
	m_PluginInfo.PortTips.push_back("Trigger 12");
	m_PluginInfo.PortTips.push_back("Trigger 13");
	m_PluginInfo.PortTips.push_back("Trigger 14");
	m_PluginInfo.PortTips.push_back("Trigger 15");
	m_PluginInfo.PortTips.push_back("Trigger 16");
	m_PluginInfo.PortTips.push_back("Reset Trigger");	
		
	for (int n=0; n<NUM_PATTERNS; n++)
	{
		m_Matrix[n].Length=64;
		m_Matrix[n].Speed=1.0f;
		m_Matrix[n].Octave=0;
		m_Matrix[n].Triggers=0;
		
		for (int x=0; x<MATX; x++)
		for (int y=0; y<MATRIX_PITCHES; y++)
		{
			m_Matrix[n].Matrix[x][y]=false;
			m_Matrix[n].Volume[x][y]=1;
			m_Matrix[n].End[x][y]=x+1;
		}

		m_TriggerLevel[n]=0;
	}

	for (int n=0; n<NUM_PATSEQ; n++)
	{
		if (n ==0) SetPatSeq(n, 0); else SetPatSeq(n, -1);
	}

	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 7);

	m_Time = FloatProperty(Property::ReadOnly, 0.0f, 0.0f, 1000.0f, 0.0001f, 0.001f);
	m_Step = SignedProperty(Property::ReadOnly, 0, 0, MATX - 1, 1, 1);
	m_Current = SignedProperty(Property::ReadOnly, 0, 0, NUM_PATTERNS - 1, 1, 1);
	m_Pattern = SignedProperty(DefaultLinearFlags, 0, 0, NUM_PATTERNS - 1, 1, 1);
	m_Length = SignedProperty(Property::ReadOnly, MATX, 1, MATX, 1, 1);
	m_Speed = FloatProperty(Property::ReadOnly, 1.0f, 0.0f, 50.0f, 0.125f, 0.125f);
	m_Octave = SignedProperty(Property::ReadOnly, 0, 0, 6, 1, 1);
	m_TriggerRow = SignedProperty(Property::ReadOnly, 0, -MATRIX_PITCHES, MATRIX_PITCHES, 1, 1);
	m_Loop = ToggleProperty(Property::WriteOnly, true);
	m_NoteCut = ToggleProperty(Property::WriteOnly, false);
	m_TriggerSelect = ToggleProperty(Property::WriteOnly, false);

	RegisterSharedProperty(m_NoteCut, "NoteCut", "Cut the note on a new step");
	RegisterSharedProperty(m_TriggerSelect, "TriggerSelect", "Trigger Select");
	m_AudioCH->Register("Volume",&m_GUIArgs.Volume,ChannelHandler::INPUT);
	m_AudioCH->Register("StepTime",&m_StepTime,ChannelHandler::INPUT);
	m_AudioCH->Register("Num",&m_GUIArgs.Num,ChannelHandler::INPUT);
	m_AudioCH->Register("SetLength",&m_GUIArgs.Length,ChannelHandler::INPUT);
	m_AudioCH->Register("SetSpeed",&m_GUIArgs.Speed,ChannelHandler::INPUT);
	m_AudioCH->Register("X",&m_GUIArgs.X,ChannelHandler::INPUT);
	m_AudioCH->Register("Y",&m_GUIArgs.Y,ChannelHandler::INPUT);
	m_AudioCH->Register("SetOctave",&m_GUIArgs.Octave,ChannelHandler::INPUT);
	m_AudioCH->Register("SetEnd",&m_GUIArgs.End,ChannelHandler::INPUT);
	RegisterSharedProperty(m_Step, "Step", "Current step");
	m_AudioCH->Register("PatSeqStep",&m_CurPatSeq,ChannelHandler::OUTPUT);
	RegisterSharedProperty(m_Current, "Current", "The pattern playing");
	RegisterSharedProperty(m_Pattern, "Pattern", "The pattern shown and edited");
	RegisterSharedProperty(m_Length, "Length", "Pattern length");
	RegisterSharedProperty(m_Speed, "Speed", "Pattern speed");
	RegisterSharedProperty(m_Octave, "Octave", "Pattern octave");
	RegisterSharedProperty(m_TriggerRow, "TriggerRow", "Trigger 1's row on the grid");

	RegisterSharedProperty(m_Time, "Time", "Position within the step");
	RegisterSharedProperty(m_Loop, "Loop", "Loop the pattern");

	RegisterSharedProperty(m_PatSeq, "Sequences", "Pattern sequence");
	m_AudioCH->RegisterData("Grid",ChannelHandler::OUTPUT,m_EchoGrid,sizeof(m_EchoGrid));
	m_AudioCH->RegisterData("GridVolume",ChannelHandler::OUTPUT,m_EchoVolume,sizeof(m_EchoVolume));
	m_AudioCH->RegisterData("PatSeq",ChannelHandler::OUTPUT,m_EchoPatSeq,sizeof(m_EchoPatSeq));
	m_AudioCH->RegisterData("GridEnd",ChannelHandler::OUTPUT,m_EchoEnd,sizeof(m_EchoEnd));
	{
		std::vector<std::string> none, args;
		m_AudioCH->RegisterCommand("CLEAR", CLEAR, none);
		m_AudioCH->RegisterCommand("COPY", COPY, none);
		args.clear(); args.push_back("X"); args.push_back("Y");
		m_AudioCH->RegisterCommand("MAT_ACTIVATE", MAT_ACTIVATE, args);
		args.clear(); args.push_back("X"); args.push_back("Y");
		m_AudioCH->RegisterCommand("MAT_DEACTIVATE", MAT_DEACTIVATE, args);
		args.clear(); args.push_back("SetLength");
		m_AudioCH->RegisterCommand("MAT_LENGTH", MAT_LENGTH, args);
		args.clear(); args.push_back("SetOctave");
		m_AudioCH->RegisterCommand("MAT_OCTAVE", MAT_OCTAVE, args);
		args.clear(); args.push_back("SetSpeed");
		m_AudioCH->RegisterCommand("MAT_SPEED", MAT_SPEED, args, "f");
		args.clear(); args.push_back("X"); args.push_back("Y"); args.push_back("Volume");
		m_AudioCH->RegisterCommand("MAT_VOLUME", MAT_VOLUME, args, "iif");
		m_AudioCH->RegisterCommand("PASTE", PASTE, none);
		args.clear(); args.push_back("Num");
		m_AudioCH->RegisterCommand("SET_CURRENT", SET_CURRENT, args);
		args.clear(); args.push_back("Y"); args.push_back("Num");
		m_AudioCH->RegisterCommand("SET_PATSEQ", SET_PATSEQ, args);
		args.clear(); args.push_back("X"); args.push_back("Y"); args.push_back("SetEnd");
		m_AudioCH->RegisterCommand("MAT_END", MAT_END, args);
		args.clear(); args.push_back("Y");
		m_AudioCH->RegisterCommand("MAT_TRIGGERS", MAT_TRIGGERS, args);
		m_AudioCH->RegisterCommand("TDOWN", TDOWN, none);
		m_AudioCH->RegisterCommand("TUP", TUP, none);
	}
	RefreshEcho();
}

MatrixPlugin::~MatrixPlugin()
{
}

PluginInfo &MatrixPlugin::Initialise(const HostInfo *Host)
{	
	PluginInfo& Info = SpiralPlugin::Initialise(Host);
	m_TickTime = 1.0f/(float)m_HostInfo->SAMPLERATE;
	return Info;
}

void MatrixPlugin::Reset()
{
	ResetPorts();
	AsSigned(m_Current) = GetPatSeq(0);
	AsSigned(m_Pattern) = 0;
	AsFloat(m_Time) = 0.0f;
	AsSigned(m_Step) = 0;
	m_TickTime = 1.0f/(float)m_HostInfo->SAMPLERATE;
	m_StepTime = 1.0f;
	m_CurPatSeq = 0;
	m_CurrentNoteCV = 0;
	m_CurrentTriggerCV = 0;
	m_Triggered = false;
	m_ClockHigh = false;
	m_CopyPattern = 0;
	m_PatAdvance = false;
	m_PatReset = false;
}

void MatrixPlugin::Execute()
{	
	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		if (InputExists(1)) m_StepTime = GetInput(1,n);
		
		// inc time 
		AsFloat(m_Time)+=m_TickTime;
		
		SetOutputPitch(0,n,m_CurrentNoteCV);
		SetOutput(1,n,m_CurrentTriggerCV);
                
		// clear the pattern sync
        SetOutput(18, n, 0);
		
		bool ExternalClock = InputExists(4);
		bool ExternalClockTriggered=false;
		 
		if (GetInputPitch(0,n)>0) 
		{
			if (!m_Triggered)
			{
				float Freq=GetInputPitch(0,n);
				// Notes 0 to 16 trigger patterns 0 to 16
				// No other notes catered for
				for (int i=0; i<NUM_PATTERNS; i++)
				{
					if (feq(Freq,NoteTable[i],0.01f))
					{						
						AsSigned(m_Current)=i;
						break;
					}
				}
				
				// make it so the next note to trigger
				// will be the first one								
				AsFloat(m_Time)=m_StepTime*(1/m_Matrix[AsSigned(m_Current)].Speed);
				AsSigned(m_Step)=-1;
				ExternalClockTriggered=true;
				m_Triggered=true;
			}
		}
		else
		{
			m_Triggered=false;
		}


		// set the individual triggers
		for (int t=0; t<NUM_PATTERNS; t++) SetOutput(t+2,n,m_TriggerLevel[t]);
		
		if (ExternalClock)
		{
			if (GetInput(4,n)>0)
			{
				if(!m_ClockHigh)
				{
					m_ClockHigh=true;
					ExternalClockTriggered=true;
				}
			}
			else
			{
				if (m_ClockHigh)
				{
					m_ClockHigh=false;
					ExternalClockTriggered=true;
				}
			}
			
			// Reset the position on a signal from input 1
			if (!AsBoolean(m_TriggerSelect) && InputExists(0) && GetInput(0,n)!=0)
			{
				AsSigned(m_Step)=-1;
				ExternalClockTriggered=true;
				m_CurPatSeq++;
				if (m_CurPatSeq>=NUM_PATSEQ || GetPatSeq(m_CurPatSeq)==-1) m_CurPatSeq=0;
				AsSigned(m_Current)=GetPatSeq(m_CurPatSeq);
			}
		}

		// An external clock pulse overrides the internal timing
		if ((!ExternalClock && AsFloat(m_Time)>=m_StepTime*(1/m_Matrix[AsSigned(m_Current)].Speed)) ||
			(ExternalClock && ExternalClockTriggered))
		{
			AsFloat(m_Time)=0;
			AsSigned(m_Step)++;

			if (AsSigned(m_Step) >= m_Matrix[AsSigned(m_Current)].Length || AsSigned(m_Step) >= MATX)
			{
				SetOutput(18, n, 1);
				AsSigned(m_Step)=0;
				m_CurPatSeq++;
				if (m_CurPatSeq>=NUM_PATSEQ || GetPatSeq(m_CurPatSeq)==-1) m_CurPatSeq=0;
				AsSigned(m_Current)=GetPatSeq(m_CurPatSeq);
			}

			// Reset the values
			m_CurrentTriggerCV=0;
			if (AsBoolean(m_NoteCut)) m_CurrentNoteCV=0;
			for (int t=0; t<NUM_PATTERNS; t++)
			{
				SetOutput(t+2,n,0);
				m_TriggerLevel[t]=0;
			}

			// Scan the matrix at current time: a note starting on this step,
			// or one carrying on from an earlier step until its end. A row
			// trigger is a pitch from the pattern's trigger rows up.
			const Pattern &p = m_Matrix[AsSigned(m_Current)];
			const int step = AsSigned(m_Step), lowest = p.Triggers;
			bool carried = false;
			for (int i=0; i<MATRIX_PITCHES; i++)
			{
				const int start = Covering(p, step, i);
				if (start < 0) continue;
				m_CurrentNoteCV=NoteTable[i];
				m_CurrentTriggerCV=p.Volume[start][i];
				if (i >= lowest && i < lowest + MATY) m_TriggerLevel[i - lowest]=p.Volume[start][i];
				carried = start != step;
			}

			// reset the triggers between steps to clear them
			// otherwise consecutive events wont get triggered;
			// a note carrying on is not triggered again
			if (!carried) SetOutput(1,n,0);
			for (int t=0; t<NUM_PATTERNS; t++)
			{
				const int start = Covering(p, step, lowest + t);
				SetOutput(t+2,n,start >= 0 && start != step ? m_TriggerLevel[t] : 0);
			}

		}
	}
}

int MatrixPlugin::Shown(int x, int y) const
{
	const int pitch = y + m_Matrix[Edited()].Octave*12;
	return x >= 0 && x < MATX && y >= 0 && y < MATY && pitch < MATRIX_PITCHES ? pitch : -1;
}

// What the grid shows of the edited pattern: its rows from its octave up.
void MatrixPlugin::RefreshEcho()
{
	const int current = Edited();
	const Pattern &p = m_Matrix[current];
	m_Echoed = current;
	AsSigned(m_Length) = p.Length;
	AsFloat(m_Speed) = p.Speed;
	AsSigned(m_Octave) = p.Octave;
	AsSigned(m_TriggerRow) = p.Triggers - p.Octave*12;
	for (int n=0; n<NUM_PATSEQ; n++) m_EchoPatSeq[n] = GetPatSeq(n);
	for (int x=0; x<MATX; x++)
		for (int y=0; y<MATY; y++)
		{
			const int pitch = Shown(x, y);
			const bool on = pitch >= 0 && p.Matrix[x][pitch];
			m_EchoGrid[x][y] = on;
			m_EchoVolume[x][y] = pitch >= 0 ? p.Volume[x][pitch] : 1.0f;
			m_EchoEnd[x][y] = on ? (unsigned char)NoteEnd(p, x, pitch) : 0;
		}
}

// Where the note starting at x stops: its own end, or the next note's start
// at its pitch, or the grid's edge, whichever comes first.
int MatrixPlugin::NoteEnd(const Pattern &p, int x, int pitch)
{
	const int end = std::max(x + 1, std::min(MATX, (int)p.End[x][pitch]));
	for (int next = x + 1; next < end; ++next)
		if (p.Matrix[next][pitch]) return next;
	return end;
}

// The start of the note sounding at step at the pitch, or -1.
int MatrixPlugin::Covering(const Pattern &p, int step, int pitch)
{
	if (step < 0 || step >= MATX) return -1;
	for (int x = step; x >= 0; --x)
		if (p.Matrix[x][pitch]) return NoteEnd(p, x, pitch) > step ? x : -1;
	return -1;
}

// Whether the panel chose another pattern, or wrote the pattern sequence:
// both are properties it writes, with no command.
bool MatrixPlugin::EchoStale()
{
	if (Edited() != m_Echoed)
		return true;
	for (int n=0; n<NUM_PATSEQ; n++)
		if (m_EchoPatSeq[n] != GetPatSeq(n))
			return true;
	return false;
}

void MatrixPlugin::ExecuteCommands()
{
	// The echo (the edited pattern's grid and volumes, 10 KB) is copied again
	// only when a command or the panel may have changed what it shows; Apply
	// refreshes it itself.
	const bool command = m_AudioCH->IsCommandWaiting();
	if (command)
	{
		// The cell the panel names is the edited pattern's, in the octave
		// its grid shows.
		const int pitch = Shown(m_GUIArgs.X, m_GUIArgs.Y);
		switch (m_AudioCH->GetCommand())
		{
			case MAT_LENGTH   :
				m_Matrix[Edited()].Length=m_GUIArgs.Length;
			break;

			case MAT_SPEED    :
				m_Matrix[Edited()].Speed=m_GUIArgs.Speed;
			break;

			case MAT_ACTIVATE :
				if (pitch >= 0)
				{
					m_Matrix[Edited()].Matrix[m_GUIArgs.X][pitch]=true;
					m_Matrix[Edited()].End[m_GUIArgs.X][pitch]=m_GUIArgs.X+1;
				}
			break;

			case MAT_END :
				if (pitch >= 0)
					m_Matrix[Edited()].End[m_GUIArgs.X][pitch]=
						(unsigned char)std::max(m_GUIArgs.X+1, std::min(MATX, m_GUIArgs.End));
			break;

			case MAT_DEACTIVATE :
				if (pitch >= 0) m_Matrix[Edited()].Matrix[m_GUIArgs.X][pitch]=false;
			break;
			// The grid shows the pattern from another octave; its notes stay
			// where they are.
			case MAT_OCTAVE :
				m_Matrix[Edited()].Octave=std::max(0, std::min(6, m_GUIArgs.Octave));
			break;
			// Trigger 1 moves to the grid's row Y, and the other fifteen
			// with it.
			case MAT_TRIGGERS :
				m_Matrix[Edited()].Triggers=std::max(0, std::min(MATRIX_PITCHES - NUM_TRIGGERS,
					m_GUIArgs.Y + m_Matrix[Edited()].Octave*12));
			break;
			case COPY :
				m_CopyPattern = Edited();
			break;
			case PASTE :
				PastePattern();
			break;
			case CLEAR :
				ClearPattern();
			break;
			case TUP :
				if (CanTransposeUp()) TransposeUp();
			break;
			case TDOWN :
				if (CanTransposeDown()) TransposeDown();
			break;
			case MAT_VOLUME :
				if (pitch >= 0) m_Matrix[Edited()].Volume[m_GUIArgs.X][pitch]=m_GUIArgs.Volume;
			break;
			case SET_CURRENT :
				AsSigned(m_Current)=m_GUIArgs.Num;
			break;
			case SET_PATSEQ :
				SetPatSeq(m_GUIArgs.Y, m_GUIArgs.Num);
			break;
		}
	}
	if (command || EchoStale())
		RefreshEcho();
}

// The patterns as the file keeps them, as the private tree does: the octave
// the grid shows, then each note at its pitch, column by column and the
// highest first, ending a step after its last at the soonest.
void MatrixPlugin::PatternsFromGrid()
{
	for (int n=0; n<NUM_PATTERNS; n++)
	{
		AsUnsigned(PatternOctave(n)) = m_Matrix[n].Octave;
		AsUnsigned(PatternLength(n)) = m_Matrix[n].Length;
		AsFloat(PatternSpeed(n)) = m_Matrix[n].Speed;
		AsUnsigned(PatternTriggers(n)) = m_Matrix[n].Triggers;
		ClearNotes(n);
		for (int x=0; x<MATX; x++)
			for (int pitch=MATRIX_PITCHES-1; pitch>=0; pitch--)
				if (m_Matrix[n].Matrix[x][pitch])
					AddNote(n, x, pitch, m_Matrix[n].Volume[x][pitch], NoteEnd(m_Matrix[n], x, pitch));
	}
}

// The patterns from the file: a note past the grid's steps, or higher than
// the Matrix plays, is left out.
void MatrixPlugin::GridFromPatterns()
{
	for (int n=0; n<NUM_PATTERNS; n++)
	{
		m_Matrix[n].Length = AsUnsigned(PatternLength(n));
		m_Matrix[n].Speed = AsFloat(PatternSpeed(n));
		m_Matrix[n].Octave = AsUnsigned(PatternOctave(n));
		m_Matrix[n].Triggers = AsUnsigned(PatternTriggers(n));
		for (int x=0; x<MATX; x++)
			for (int y=0; y<MATRIX_PITCHES; y++)
			{
				m_Matrix[n].Matrix[x][y] = false;
				m_Matrix[n].End[x][y] = x + 1;
			}
		for (int k=0; k<NoteCount(n); k++)
		{
			const int x = (int)Note(n, k).Start, pitch = NotePitch(Note(n, k));
			if (x < 0 || x >= MATX || pitch < 0 || pitch >= MATRIX_PITCHES)
				continue;
			m_Matrix[n].Matrix[x][pitch] = true;
			m_Matrix[n].Volume[x][pitch] = Note(n, k).Velocity;
			m_Matrix[n].End[x][pitch] = (unsigned char)std::max(x + 1, std::min(MATX, (int)Note(n, k).End));
		}
	}
}

void MatrixPlugin::SetPitch(NoteProperty &note, int pitch)
{
	pitch = std::max(0, pitch);
	note.Note = (UnsignedType)(pitch % 12 + 1);
	note.Octave = (UnsignedType)(pitch / 12);
}

void MatrixPlugin::AddNote(int pattern, int x, int pitch, float volume, int end)
{
	NoteProperty *note = new NoteProperty(DefaultLinearFlags, 1, 0, volume);
	SetPitch(*note, pitch);
	note->Start = (UnsignedType)x;
	note->End = (UnsignedType)std::max(x + 1, end);
	PatternProperties(pattern).Add(note);
}

void MatrixPlugin::ClearNotes(int pattern)
{
	while (NoteCount(pattern) > 0) PatternProperties(pattern).RemoveLast();
}

void MatrixPlugin::PastePattern() {
     const Pattern &from = m_Matrix[m_CopyPattern];
     Pattern &to = m_Matrix[Edited()];
     to.Length = from.Length;
     to.Speed = from.Speed;
     to.Octave = from.Octave;
     to.Triggers = from.Triggers;
     for (int y=0; y<MATRIX_PITCHES; y++) {
         for (int x=0; x<MATX; x++) {
             to.Matrix[x][y] = from.Matrix[x][y];
             to.End[x][y] = from.End[x][y];
         }
     }
}

void MatrixPlugin::ClearPattern() {
     for (int y=0; y<MATRIX_PITCHES; y++) {
         for (int x=0; x<MATX; x++) {
             m_Matrix[Edited()].Matrix[x][y] = 0;
             m_Matrix[Edited()].End[x][y] = x + 1;
         }
     }
}

// Every note of the pattern moves, shown or not, with its volume and end.
void MatrixPlugin::TransposeUp() {
     Pattern &p = m_Matrix[Edited()];
     int x, y;
     for (y=MATRIX_PITCHES-1; y>0; y--) {
         for (x=0; x<MATX; x++) {
             p.Matrix[x][y] = p.Matrix[x][y-1];
             p.Volume[x][y] = p.Volume[x][y-1];
             p.End[x][y] = p.End[x][y-1];
         }
     }
     for (x=0; x<MATX; x++) {
         p.Matrix[x][0] = 0;
         p.End[x][0] = x + 1;
     }
}

void MatrixPlugin::TransposeDown()
{
    Pattern &p = m_Matrix[Edited()];
    int x, y;
    for (y=0; y<MATRIX_PITCHES-1; y++)
	{
        for (x=0; x<MATX; x++)
		{
            p.Matrix[x][y] = p.Matrix[x][y+1];
            p.Volume[x][y] = p.Volume[x][y+1];
            p.End[x][y] = p.End[x][y+1];
        }
    }
    for (x=0; x<MATX; x++)
	{
    	p.Matrix[x][MATRIX_PITCHES-1] = 0;
    	p.End[x][MATRIX_PITCHES-1] = x + 1;
    }
}

bool MatrixPlugin::CanTransposeDown()
{
    for (int x=0; x<MATX; x++) if (m_Matrix[Edited()].Matrix[x][0]) return false;
    return true;
}

bool MatrixPlugin::CanTransposeUp()
{
     for (int x=0; x<MATX; x++) if (m_Matrix[Edited()].Matrix[x][MATRIX_PITCHES-1]) return false;
     return true;
}

// What the private tree saves of a Matrix, in its order: whether a step cuts
// the note, the patterns, the pattern sequence and Trigger Select, each list
// and pattern under the class and name it gives them. Where it is playing is
// not saved.
void MatrixPlugin::Describe(Spiral::Core::Description &d)
{
	PatternsFromGrid();

	d.Value(DeviceVersion).Separator(" ");
	d.Value(m_NoteCut).Separator(" ").Line();

	d.Shaped(m_Patterns.GetKey(), m_Patterns.ClassName(), "");
	for (int n=0; n<NUM_PATTERNS; n++)
	{
		VectorProperty &pattern = PatternProperties(n);
		d.Shaped(pattern.GetKey(), pattern.ClassName(), pattern.GetLabel());
		for (int f=0; f<PatternFields; f++)
			d.Shaped(pattern[f]->GetKey(), "", pattern[f]->GetLabel());
		d.Value(PatternOctave(n)).Separator(" ").Value(PatternLength(n)).Separator(" ").Value(PatternSpeed(n))
			.Separator(" ").Value(PatternTriggers(n)).Line();
		for (int k=0; k<NoteCount(n); k++)
			d.Value(Note(n, k)).Separator(" ");
		d.Line();
	}

	d.Shaped(m_PatSeq.GetKey(), m_PatSeq.ClassName(), "");
	for (int n=0; n<NUM_PATSEQ; n++)
	{
		d.Shaped(PatSeq(n).GetKey(), "", PatSeq(n).GetLabel());
		d.Value(PatSeq(n)).Separator(" ");
	}
	d.Line();
	d.Value(m_TriggerSelect);
}

static bool InGrid(int x, int y) { return x >= 0 && x < MATX && y >= 0 && y < MATY; }

void MatrixPlugin::Apply(Spiral::Core::Description::Reader &r) {
     int version;
     r.Value(version);
     if (r.Named()) {
        ApplyNamed(r, version);
        return;
     }
     // A legacy stream, which kept where the Matrix was playing.
     r.Value(m_Current).Value(m_Time).Value(m_Step).Value(m_Loop).Value(m_NoteCut);
     if (version > 4) r.Value(m_TriggerSelect);
     for (int n=0; n<NUM_PATTERNS; n++) {
         r.Value(PatternLength(n)).Value(PatternSpeed(n)).Value(PatternOctave(n));
         AsUnsigned(PatternLength(n)) = std::max(1, std::min(MATX, (int)AsUnsigned(PatternLength(n))));
         AsUnsigned(PatternOctave(n)) = std::min(6u, (unsigned)AsUnsigned(PatternOctave(n)));
         m_Matrix[n].Length = AsUnsigned(PatternLength(n));
         m_Matrix[n].Speed = AsFloat(PatternSpeed(n));
         m_Matrix[n].Octave = AsUnsigned(PatternOctave(n));
         // Its rows were the pattern's octave up, and its trigger rows the
         // first sixteen of them.
         const int lowest = m_Matrix[n].Octave*12;
         m_Matrix[n].Triggers = lowest;
         AsUnsigned(PatternTriggers(n)) = lowest;
         for (int x=0; x<MATX; x++)
             for (int y=0; y<MATRIX_PITCHES; y++)
                 m_Matrix[n].End[x][y] = x + 1;
         if (version > 5) {
            // Version 6 kept its notes' octaves under the pattern's.
            int count = 0;
            r.Value(count);
            ClearNotes(n);
            for (int k=0; k<count && r.More(); k++) {
                AddNote(n, 0, 0, 1.0f, 1);
                r.Value(Note(n, k));
                SetPitch(Note(n, k), NotePitch(Note(n, k)) + m_Matrix[n].Octave*12);
            }
         }
         else if (version == 1) {
            for (int y=0; y<MATY; y++)
                for (int x=0; x<MATX; x++)
                    r.Value(m_Matrix[n].Matrix[x][lowest + y]);
         }
         else {
            // version > 1
            float vol = 1.0f;
            int x=0;
            while (x!=-1 && r.More()) {
                  int y = -1;
                  r.Value(x);
                  if (x != -1) {
                     if (version == 2) {
                        r.Value(y);
                        if (InGrid(x, y)) m_Matrix[n].Matrix[x][lowest + y] = true;
                     }
                     else {
                        // version > 2
                        r.Value(y).Value(vol);
                        if (InGrid(x, y)) {
                           m_Matrix[n].Matrix[x][lowest + y] = true;
                           m_Matrix[n].Volume[x][lowest + y] = vol;
                        }
                     }
                  }
            }
         }
     }
     if (version > 5) GridFromPatterns();
     else PatternsFromGrid();
     if (version > 3) {
        int kept[NUM_PATSEQ];
        bool valid = true;
        for (int n=0; n<NUM_PATSEQ; n++) {
            kept[n] = GetPatSeq(n);
            r.Value(PatSeq(n));
            valid = valid && GetPatSeq(n) >= -1 && GetPatSeq(n) < NUM_PATTERNS;
        }
        for (int n=0; n<NUM_PATSEQ && !valid; n++) SetPatSeq(n, kept[n]);
        if (GetPatSeq(0)<0) SetPatSeq(0, 0);
     }
     AsSigned(m_Current) = std::max(0, std::min(NUM_PATTERNS - 1, (int)AsSigned(m_Current)));
     AsSigned(m_Step) = std::max(-1, std::min(MATX - 1, (int)AsSigned(m_Step)));
     RefreshEcho();
}

// A file's Matrix. Version 7 is what both trees write. Before it, the private
// tree wrote the patterns as now without their trigger rows, its notes
// counted from 0, with 12 the next C.
void MatrixPlugin::ApplyNamed(Spiral::Core::Description::Reader &r, int version)
{
	r.Value(m_NoteCut).Value(m_TriggerSelect);
	for (int n=0; n<NUM_PATTERNS; n++)
	{
		r.Value(PatternOctave(n)).Value(PatternLength(n)).Value(PatternSpeed(n));
		AsUnsigned(PatternOctave(n)) = std::min(6u, (unsigned)AsUnsigned(PatternOctave(n)));
		// Its trigger rows; a file from before them has its first note
		// there, and its triggers were the sixteen rows from its octave.
		const bool marked = version >= 7 && r.Find(PatternTriggers(n).GetKey()) >= 0;
		AsUnsigned(PatternTriggers(n)) = AsUnsigned(PatternOctave(n)) * 12;
		if (marked)
			r.Value(PatternTriggers(n));
		AsUnsigned(PatternTriggers(n)) = std::min((unsigned)(MATRIX_PITCHES - NUM_TRIGGERS), (unsigned)AsUnsigned(PatternTriggers(n)));
		int count = 0;
		r.Count(PatternProperties(n), count);
		ClearNotes(n);
		for (int k = marked ? PatternFields : PatternFields - 1; k<count; k++)
		{
			char key[64];
			snprintf(key, sizeof key, "%s[%d]", PatternProperties(n).GetKey().c_str(), k);
			NoteProperty note(DefaultLinearFlags, 1, 0, 1.0f);
			note.SetKey(key);
			r.Value(note);
			if (version < 7)
				SetPitch(note, (int)note.Note + (int)note.Octave*12);
			if (NotePitch(note) >= 0)
				AddNote(n, (int)note.Start, NotePitch(note), note.Velocity, (int)note.End);
		}
		AsUnsigned(PatternLength(n)) = std::max(1, std::min(MATX, (int)AsUnsigned(PatternLength(n))));
	}
	GridFromPatterns();

	int kept[NUM_PATSEQ];
	bool valid = true;
	for (int n=0; n<NUM_PATSEQ; n++)
	{
		kept[n] = GetPatSeq(n);
		r.Value(PatSeq(n));
		valid = valid && GetPatSeq(n) >= -1 && GetPatSeq(n) < NUM_PATTERNS;
	}
	for (int n=0; n<NUM_PATSEQ && !valid; n++) SetPatSeq(n, kept[n]);
	if (GetPatSeq(0)<0) SetPatSeq(0, 0);
	RefreshEcho();
}
