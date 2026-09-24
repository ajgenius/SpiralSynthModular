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

#include "SpiralPlugin.h"
#include <FL/Fl_Pixmap.H>

#ifndef SamplerPLUGIN
#define SamplerPLUGIN

static const int NUM_SAMPLES = 8;

struct SampleDesc
{
	std::string Pathname;
	float  Volume;
	float  Pitch;
	float  PitchMod;
	bool   Loop;
	int    Note;
	bool   TriggerUp;	
	float  SamplePos;
	int    SampleRate;
	bool   Stereo;
};

class SamplerPlugin : public SpiralPlugin
{
public:
 	SamplerPlugin();
	virtual ~SamplerPlugin();
	
	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual SpiralGUIType *CreateGUI();
	virtual void Execute();
	virtual void StreamOut(std::ostream &s);
	virtual void StreamIn(std::istream &s);
	
	// has to be defined in the plugin	
	virtual void UpdateGUI() { Fl::check(); }
	
	void LoadSample(int n, const std::string &Name);
	
	void  SetVolume(int n, float s) { m_SampleDescVec[n]->Volume=s; }
	float GetVolume(int n)          { return m_SampleDescVec[n]->Volume; }
	void  SetPitch(int n, float s)  { m_SampleDescVec[n]->PitchMod=s; }
	float GetPitch(int n)           { return m_SampleDescVec[n]->PitchMod; }
	void  SetLoop(int n, bool s)    { m_SampleDescVec[n]->Loop=s; }
	bool  GetLoop(int n)            { return m_SampleDescVec[n]->Loop; }
	void  SetNote(int n, int s)     { m_SampleDescVec[n]->Note=s; }
	int   GetNote(int n)            { return m_SampleDescVec[n]->Note; }
		
	std::vector<Sample*> m_SampleVec;
	std::vector<SampleDesc*> m_SampleDescVec;
			
private:
};

#endif
