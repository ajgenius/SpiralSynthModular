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
#include "MatrixPlugin.h"
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

MatrixPlugin::MatrixPlugin() :
m_TickTime(1.0f),
m_StepTime(1.0f),
m_GUICurrent(0),
m_CurPatSeq(0),
m_CurrentNoteCV(0),
m_CurrentTriggerCV(0),
m_Triggered(false),
m_ClockHigh(false),
m_CopyPattern(0),
m_PatAdvance(false),
m_PatReset(false)
{
	m_PatSeq = VectorProperty(Property::WriteOnly);
	for (int n=0; n<NUM_PATSEQ; n++)
		m_PatSeq.Add(new SignedProperty(DefaultLinearFlags, -1, -1, NUM_PATTERNS - 1, 1, 1));

	m_Patterns = VectorProperty(Property::WriteOnly);
	for (int n=0; n<NUM_PATTERNS; n++)
	{
		VectorProperty *pattern = new VectorProperty(Property::WriteOnly);
		pattern->Add(new SignedProperty(DefaultLinearFlags, 64, 1, MATX, 1, 1));
		pattern->Add(new FloatProperty(DefaultLinearFlags, 1.0f, 0.0f, 16.0f, 0.001f, 0.1f));
		pattern->Add(new SignedProperty(DefaultLinearFlags, 0, -6, 6, 1, 1));
		pattern->Add(new VectorProperty(Property::WriteOnly));
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
		
		for (int x=0; x<MATX; x++)
		for (int y=0; y<MATY; y++)
		{
			m_Matrix[n].Matrix[x][y]=false;
			m_Matrix[n].Volume[x][y]=1;
		}

		m_TriggerLevel[n]=0;
	}

	for (int n=0; n<NUM_PATSEQ; n++)
	{
		if (n ==0) SetPatSeq(n, 0); else SetPatSeq(n, -1);
	}

	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 4);

	m_Time = FloatProperty(Property::ReadOnly, 0.0f, 0.0f, 1000.0f, 0.0001f, 0.001f);
	m_Step = SignedProperty(Property::ReadOnly, 0, 0, MATX - 1, 1, 1);
	m_Current = SignedProperty(Property::ReadOnly, 0, 0, NUM_PATTERNS - 1, 1, 1);
	m_Loop = ToggleProperty(Property::WriteOnly, true);
	m_NoteCut = ToggleProperty(Property::WriteOnly, false);

	RegisterSharedProperty(m_NoteCut, "NoteCut", "Cut the note on a new step");
	m_AudioCH->Register("Volume",&m_GUIArgs.Volume,ChannelHandler::INPUT);
	m_AudioCH->Register("Current",&m_GUICurrent,ChannelHandler::INPUT);
	m_AudioCH->Register("StepTime",&m_StepTime,ChannelHandler::INPUT);
	m_AudioCH->Register("Num",&m_GUIArgs.Num,ChannelHandler::INPUT);
	m_AudioCH->Register("Length",&m_GUIArgs.Length,ChannelHandler::INPUT);
	m_AudioCH->Register("Speed",&m_GUIArgs.Speed,ChannelHandler::INPUT);
	m_AudioCH->Register("X",&m_GUIArgs.X,ChannelHandler::INPUT);
	m_AudioCH->Register("Y",&m_GUIArgs.Y,ChannelHandler::INPUT);
	m_AudioCH->Register("Octave",&m_GUIArgs.Octave,ChannelHandler::INPUT);
	RegisterSharedProperty(m_Step, "Step", "Current step");
	m_AudioCH->Register("PatSeqStep",&m_CurPatSeq,ChannelHandler::OUTPUT);
	RegisterSharedProperty(m_Current, "EchoCur", "Current pattern");

	RegisterSharedProperty(m_Time, "Time", "Position within the step");
	RegisterSharedProperty(m_Loop, "Loop", "Loop the pattern");

	RegisterSharedProperty(m_PatSeq, "PatSeq", "Pattern sequence");
	m_AudioCH->RegisterData("Matrix",ChannelHandler::OUTPUT_REQUEST,&m_Matrix,sizeof(m_Matrix));
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
	m_GUICurrent = 0;
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
			
			// reset the position on a signal from input 1
			if (InputExists(0) && GetInput(0,n)!=0)
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

			// Scan the matrix at current time
			for (int i=0; i<MATY; i++)
			{
				if (m_Matrix[AsSigned(m_Current)].Matrix[AsSigned(m_Step)][i])
				{
					m_CurrentNoteCV=NoteTable[i+m_Matrix[AsSigned(m_Current)].Octave*12];
					m_CurrentTriggerCV=m_Matrix[AsSigned(m_Current)].Volume[AsSigned(m_Step)][i];
					m_TriggerLevel[i]=m_Matrix[AsSigned(m_Current)].Volume[AsSigned(m_Step)][i];
				}
			}

			// reset the triggers between steps to clear them
			// otherwise consecutive events wont get triggered
			SetOutput(1,n,0);
			for (int t=0; t<NUM_PATTERNS; t++) SetOutput(t+2,n,0);
		}
	}
}

void MatrixPlugin::ExecuteCommands()
{
	if (m_AudioCH->IsCommandWaiting())
	{
		switch (m_AudioCH->GetCommand())
		{
			case MAT_LENGTH   :
				m_Matrix[m_GUICurrent].Length=m_GUIArgs.Length;
			break;

			case MAT_SPEED    :
				m_Matrix[m_GUICurrent].Speed=m_GUIArgs.Speed;
			break;

			case MAT_ACTIVATE :
				m_Matrix[m_GUICurrent].Matrix[m_GUIArgs.X][m_GUIArgs.Y]=true;
			break;

			case MAT_DEACTIVATE :
				m_Matrix[m_GUICurrent].Matrix[m_GUIArgs.X][m_GUIArgs.Y]=false;
			break;
			case MAT_OCTAVE :
				m_Matrix[m_GUICurrent].Octave=m_GUIArgs.Octave;
			break;
			case COPY :
                                m_CopyPattern = m_GUICurrent;
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
				m_Matrix[m_GUICurrent].Volume[m_GUIArgs.X][m_GUIArgs.Y]=m_GUIArgs.Volume;
			break;
			case SET_CURRENT :
				AsSigned(m_Current)=m_GUIArgs.Num;
			break;
			case SET_PATSEQ :
				SetPatSeq(m_GUIArgs.Y, m_GUIArgs.Num);
			break;
		}
	}
}

void MatrixPlugin::PatternsFromGrid()
{
	for (int n=0; n<NUM_PATTERNS; n++)
	{
		AsSigned(PatternLength(n)) = m_Matrix[n].Length;
		AsFloat(PatternSpeed(n)) = m_Matrix[n].Speed;
		AsSigned(PatternOctave(n)) = m_Matrix[n].Octave;
		ClearNotes(n);
		for (int y=0; y<MATY; y++)
			for (int x=0; x<MATX; x++)
				if (m_Matrix[n].Matrix[x][y]) AddNote(n, x, y, m_Matrix[n].Volume[x][y]);
	}
}

void MatrixPlugin::GridFromPatterns()
{
	for (int n=0; n<NUM_PATTERNS; n++)
	{
		m_Matrix[n].Length = AsSigned(PatternLength(n));
		m_Matrix[n].Speed = AsFloat(PatternSpeed(n));
		m_Matrix[n].Octave = AsSigned(PatternOctave(n));
		for (int x=0; x<MATX; x++)
			for (int y=0; y<MATY; y++)
				m_Matrix[n].Matrix[x][y] = false;
		for (unsigned int k=0; k<Notes(n).size(); k++)
		{
			const int x = AsSigned(NoteX(n, k)), y = AsSigned(NoteY(n, k));
			if (x < 0 || x >= MATX || y < 0 || y >= MATY) continue;
			m_Matrix[n].Matrix[x][y] = true;
			m_Matrix[n].Volume[x][y] = AsFloat(NoteVolume(n, k));
		}
	}
}

void MatrixPlugin::AddNote(int pattern, int x, int y, float volume)
{
	VectorProperty *note = new VectorProperty(Property::WriteOnly);
	note->Add(new SignedProperty(DefaultLinearFlags, x, 0, MATX - 1, 1, 1));
	note->Add(new SignedProperty(DefaultLinearFlags, y, 0, MATY - 1, 1, 1));
	note->Add(new FloatProperty(DefaultLinearFlags, volume, 0.0f, 1.0f, 0.001f, 0.1f));
	Notes(pattern).Add(note);
}

void MatrixPlugin::ClearNotes(int pattern)
{
	while (Notes(pattern).size()) Notes(pattern).RemoveLast();
}

void MatrixPlugin::PastePattern() {
     m_Matrix[m_GUICurrent].Length = m_Matrix[m_CopyPattern].Length;
     m_Matrix[m_GUICurrent].Speed = m_Matrix[m_CopyPattern].Speed;
     m_Matrix[m_GUICurrent].Octave = m_Matrix[m_CopyPattern].Octave;
     for (int y=0; y<MATY; y++) {
         for (int x=0; x<MATX; x++) {
             m_Matrix[m_GUICurrent].Matrix[x][y] = m_Matrix[m_CopyPattern].Matrix[x][y];
         }
     }
}

void MatrixPlugin::ClearPattern() {
     for (int y=0; y<MATY; y++) {
         for (int x=0; x<MATX; x++) {
             m_Matrix[m_GUICurrent].Matrix[x][y] = 0;
         }
     }
}

void MatrixPlugin::TransposeUp() {
     int x, y;
     for (y=MATY-1; y>=0; y--) {
         for (x=0; x<MATX; x++) {
             m_Matrix[m_GUICurrent].Matrix[x][y] = m_Matrix[m_GUICurrent].Matrix[x][y-1];
         }
     }
     for (x=0; x<MATX; x++) {
         m_Matrix[m_GUICurrent].Matrix[x][0] = 0;
     }
}

void MatrixPlugin::TransposeDown()
{
    int x, y;
    for (y=0; y<MATY-1; y++)
	{
        for (x=0; x<MATX; x++)
		{
            m_Matrix[m_GUICurrent].Matrix[x][y] = m_Matrix[m_GUICurrent].Matrix[x][y+1];
        }
    }
    for (x=0; x<MATX; x++)
	{
    	m_Matrix[m_GUICurrent].Matrix[x][MATY-1] = 0;
    }
}

bool MatrixPlugin::CanTransposeDown()
{
    for (int x=0; x<MATX; x++) if (m_Matrix[m_GUICurrent].Matrix[x][0]) return false;
    return true;
}

bool MatrixPlugin::CanTransposeUp()
{
     for (int x=0; x<MATX; x++) if (m_Matrix[AsSigned(m_Current)].Matrix[x][MATY-1]) return false;
     return true;
}

void MatrixPlugin::Describe(Spiral::Core::Description &d)
{
	PatternsFromGrid();

	d.Value(DeviceVersion).Separator(" ");
	d.Value(m_Current).Separator(" ").Value(m_Time).Separator(" ").Value(m_Step).Separator(" ").Value(m_Loop).Separator(" ").Value(m_NoteCut).Separator(" ").Line();

	for (int n=0; n<NUM_PATTERNS; n++)
	{
		d.Value(PatternLength(n)).Separator(" ").Value(PatternSpeed(n)).Separator(" ").Value(PatternOctave(n)).Line();

		for (unsigned int k=0; k<Notes(n).size(); k++)
		{
			d.Value(NoteX(n, k)).Separator(" ").Value(NoteY(n, k)).Separator("  ").Value(NoteVolume(n, k)).Separator(" ");
		}
		d.Value("-1").Separator(" ");
	}
	d.Line();
	for (int n=0; n<NUM_PATSEQ; n++)
	{
		d.Value(PatSeq(n)).Separator(" ");
	}

}

void MatrixPlugin::Apply(Spiral::Core::Description::Reader &r) {
     int version;
     r.Value(version);
     r.Value(m_Current).Value(m_Time).Value(m_Step).Value(m_Loop).Value(m_NoteCut);
     for (int n=0; n<NUM_PATTERNS; n++) {
         r.Value(PatternLength(n)).Value(PatternSpeed(n)).Value(PatternOctave(n));
         m_Matrix[n].Length = AsSigned(PatternLength(n));
         m_Matrix[n].Speed = AsFloat(PatternSpeed(n));
         m_Matrix[n].Octave = AsSigned(PatternOctave(n));
         if (version == 1) {
            for (int y=0; y<MATY; y++)
                for (int x=0; x<MATX; x++)
                    r.Value(m_Matrix[n].Matrix[x][y]);
         }
         else {
            // version > 1
            float vol;
            int x=0, y=0;
            while (x!=-1) {
                  r.Value(x);
                  if (x != -1) {
                     if (version == 2) {
                        r.Value(y);
                        if (y != -1) m_Matrix[n].Matrix[x][y] = true;
                     }
                     else {
                        // version > 2
                        r.Value(y).Value(vol);
                        if (y != -1) {
                           m_Matrix[n].Matrix[x][y] = true;
                           m_Matrix[n].Volume[x][y] = vol;
                        }
                     }
                  }
            }
         }
     }
     PatternsFromGrid();
     if (version > 3) {
        for (int n=0; n<NUM_PATSEQ; n++) {
            r.Value(PatSeq(n));
            if (n==0 && GetPatSeq(0)<0) SetPatSeq(0, 0);
        }
     }
}
