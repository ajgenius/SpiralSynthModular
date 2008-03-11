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
#include "SamplerPlugin.h"
#include "SamplerPluginGUI.h"
#include <Fl/Fl_Button.H>
#include "SpiralIcon.xpm"
#include "../../RiffWav.h"
#include "../../NoteTable.h"

using namespace std;

static const int   NOTETRIG    = NUM_SAMPLES*2+1;

extern "C" {
SpiralPlugin* SpiralPlugin_CreateInstance()
{
	return new SamplerPlugin;
}

char** SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}

int SpiralPlugin_GetID()
{
	return 0x0010;
}

string SpiralPlugin_GetGroupName()
{
	return "Delay/Sampling";
}
}

///////////////////////////////////////////////////////

SamplerPlugin::SamplerPlugin()
{
	m_PluginInfo.Name="Sampler";
	m_PluginInfo.Width=245;
	m_PluginInfo.Height=420;
	m_PluginInfo.NumInputs=18;
	m_PluginInfo.NumOutputs=9;
	m_PluginInfo.PortTips.push_back("Sample 1 Pitch");	
	m_PluginInfo.PortTips.push_back("Sample 1 Trigger");	
	m_PluginInfo.PortTips.push_back("Sample 2 Pitch");	
	m_PluginInfo.PortTips.push_back("Sample 2 Trigger");	
	m_PluginInfo.PortTips.push_back("Sample 3 Pitch");	
	m_PluginInfo.PortTips.push_back("Sample 3 Trigger");	
	m_PluginInfo.PortTips.push_back("Sample 4 Pitch");	
	m_PluginInfo.PortTips.push_back("Sample 4 Trigger");	
	m_PluginInfo.PortTips.push_back("Sample 5 Pitch");	
	m_PluginInfo.PortTips.push_back("Sample 5 Trigger");	
	m_PluginInfo.PortTips.push_back("Sample 6 Pitch");	
	m_PluginInfo.PortTips.push_back("Sample 6 Trigger");	
	m_PluginInfo.PortTips.push_back("Sample 7 Pitch");	
	m_PluginInfo.PortTips.push_back("Sample 7 Trigger");	
	m_PluginInfo.PortTips.push_back("Sample 8 Pitch");	
	m_PluginInfo.PortTips.push_back("Sample 8 Trigger");	
	m_PluginInfo.PortTips.push_back("Input");
	m_PluginInfo.PortTips.push_back("Sample trigger pitch");	
	m_PluginInfo.PortTips.push_back("Mixed Output");	
	m_PluginInfo.PortTips.push_back("Sample 1 Output");
	m_PluginInfo.PortTips.push_back("Sample 2 Output");
	m_PluginInfo.PortTips.push_back("Sample 3 Output");
	m_PluginInfo.PortTips.push_back("Sample 4 Output");
	m_PluginInfo.PortTips.push_back("Sample 5 Output");
	m_PluginInfo.PortTips.push_back("Sample 6 Output");
	m_PluginInfo.PortTips.push_back("Sample 7 Output");
	m_PluginInfo.PortTips.push_back("Sample 8 Output");
	
	for (int n=0; n<NUM_SAMPLES; n++)
	{
		Sample* NewSample = new Sample;
		m_SampleVec.push_back(NewSample);
		
		SampleDesc* NewDesc = new SampleDesc;
						
		NewDesc->Volume     = 1.0f;
		NewDesc->Pitch      = 1.0f;
		NewDesc->PitchMod   = 1.0f;
		NewDesc->SamplePos  = -1;
		NewDesc->Loop       = false;	
		NewDesc->Note       = n;	
		NewDesc->Pathname   = "None";
		NewDesc->TriggerUp  = true;
		NewDesc->SampleRate = 44100;
		NewDesc->Stereo     = false;
		
		m_SampleDescVec.push_back(NewDesc);
	}
	
	m_Version=2;
}

SamplerPlugin::~SamplerPlugin()
{
	for (vector<Sample*>::iterator i=m_SampleVec.begin();
		 i!=m_SampleVec.end(); i++)
	{
		delete(*i);
	}
	
	for (vector<SampleDesc*>::iterator i=m_SampleDescVec.begin();
		 i!=m_SampleDescVec.end(); i++)
	{
		delete(*i);
	}
}

PluginInfo &SamplerPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);;
}

SpiralGUIType *SamplerPlugin::CreateGUI()
{
	m_GUI = new SamplerPluginGUI(m_PluginInfo.Width,
								  	    m_PluginInfo.Height,
										this,m_HostInfo);
	m_GUI->hide();
	return m_GUI;
}

void SamplerPlugin::Execute()
{
	for (int s=0; s<NUM_SAMPLES+1; s++)
	{
		GetOutputBuf(s)->Zero();
	}

	float Freq=0;

	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		Freq=GetInputPitch(NOTETRIG,n);
	
		for (int s=0; s<NUM_SAMPLES; s++)
		{	
			SampleDesc* S=m_SampleDescVec[s];
		
			// if we have a sample here
			if (m_SampleVec[s]->GetLength())
			{
				// Convert the CV input into a useable trigger
					
				if (GetInput(s*2+1,n)>0 || feq(Freq,NoteTable[S->Note],0.01f))
				{			
					if (S->TriggerUp)
					{
						S->SamplePos=0;
						S->TriggerUp=false;
						
						if (InputExists(s*2))
						{
							// Get the pitch from the CV
							float PlayFreq=GetInputPitch(s*2,n);
						
							// assumtion: base frequency = 440 (middle A)
							S->Pitch = PlayFreq/440;														
							S->Pitch *= S->SampleRate/(float)m_HostInfo->SAMPLERATE;
														
							// sort of legacy 
							if (S->Stereo) S->Pitch/=2;																																	
						}
					}
				}
				else 
				{
					S->TriggerUp=true;
					
					// end it if it's looping
					if (S->Loop)
					{
						S->SamplePos=-1;
					}
				}
				
				// if the sample has ended
				if (S->SamplePos>=m_SampleVec[s]->GetLength())
				{
					if (S->Loop) S->SamplePos=0;
					else 
					{
						S->SamplePos=-1;
					}
				}
			
				if (S->SamplePos!=-1)			
				{											
					// mix the sample to the output.
					MixOutput(0,n,(*m_SampleVec[s])[S->SamplePos]*S->Volume);											
					// copy the sample to it's individual output.
					SetOutput(s+1,n,((*m_SampleVec[s])[S->SamplePos]*S->Volume));	

					S->SamplePos+=S->Pitch*S->PitchMod;
				}						
			}
		}
	}
}

void SamplerPlugin::StreamOut(ostream &s)
{
	s<<m_Version<<" ";
	if (m_Version>1)
	{
		for (int n=0; n<NUM_SAMPLES; n++)
		{	
			s<<m_SampleDescVec[n]->Volume<<" "<<
			m_SampleDescVec[n]->PitchMod<<" "<<
			m_SampleDescVec[n]->Pathname.size()<<" "<<
			m_SampleDescVec[n]->Pathname<<" "<<
			m_SampleDescVec[n]->Note<<endl;
		}
	}
	else
	{
		for (int n=0; n<NUM_SAMPLES; n++)
		{	
			s<<m_SampleDescVec[n]->Volume<<" "<<
			m_SampleDescVec[n]->PitchMod<<" "<<
			m_SampleDescVec[n]->Pathname<<" "<<
			m_SampleDescVec[n]->Note<<endl;
		}
	}
}

void SamplerPlugin::StreamIn(istream &s)
{
	int version;
	s>>version;
	if (version>1)
	{
		char Buf[4096];
		
		for (int n=0; n<NUM_SAMPLES; n++)
		{
			s>>m_SampleDescVec[n]->Volume>>
			m_SampleDescVec[n]->PitchMod;
			
			int size;		
			s>>size;
			s.ignore(1);
			s.get(Buf,size+1);
			m_SampleDescVec[n]->Pathname=Buf;
			
			s>>m_SampleDescVec[n]->Note;
			if (m_SampleDescVec[n]->Pathname!="None") LoadSample(n, m_SampleDescVec[n]->Pathname);
		}
	}
	else
	{
		for (int n=0; n<NUM_SAMPLES; n++)
		{
			s>>m_SampleDescVec[n]->Volume>>
			m_SampleDescVec[n]->PitchMod>>
			m_SampleDescVec[n]->Pathname>>
			m_SampleDescVec[n]->Note;
			if (m_SampleDescVec[n]->Pathname!="None") LoadSample(n, m_SampleDescVec[n]->Pathname);
		}
	}
}

void SamplerPlugin::LoadSample(int n, const string &Name)
{
	WavFile Wav;
	if (Wav.Open(Name,WavFile::READ))
	{
		m_SampleVec[n]->Allocate(Wav.GetSize());
		Wav.Load(*m_SampleVec[n]);
		m_SampleDescVec[n]->Pathname=Name;
		m_SampleDescVec[n]->SampleRate=Wav.GetSamplerate();
		m_SampleDescVec[n]->Stereo=Wav.IsStereo();
		m_SampleDescVec[n]->Pitch *= m_SampleDescVec[n]->SampleRate/(float)m_HostInfo->SAMPLERATE;												
	}
}
