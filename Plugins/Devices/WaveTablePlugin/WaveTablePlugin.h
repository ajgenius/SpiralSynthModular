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

#ifndef WaveTablePLUGIN
#define WaveTablePLUGIN

static const int NUM_TABLES = 8;
static const int DEFAULT_TABLE_LEN = 1024;

class WaveTablePlugin : public Spiral::Core::Device
{
public:
 	WaveTablePlugin();
	virtual ~WaveTablePlugin();

	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void 		Execute();
	virtual void		Reset();

	virtual void	    Describe(Spiral::Core::Description &d);
	virtual void	    Apply(Spiral::Core::Description::Reader &r);
	virtual void	    Migrate(Spiral::Core::Description &d);
	
	// has to be defined in the plugin	

	typedef char Type;
	enum {SINE,SQUARE,SAW,REVSAW,TRIANGLE,PULSE1,PULSE2,INVSINE};
	enum {OCTAVE_ORIGIN=3};
	
	void WriteWaves();
	void NoteTrigger(int V,int s,int v);
	
	Type GetType()      { return (Type)AsUnsigned(m_Type); }
	int  GetOctave()    { return AsSigned(m_Octave); }
	int  OctaveShift()  { return AsSigned(m_Octave)-OCTAVE_ORIGIN; }
	float GetFineFreq() { return AsFloat(m_FineFreq); } 
	float GetModAmount(){ return AsFloat(m_ModAmount); }
	
private:

	// Voice specific parameter
	int   m_Note;
	float m_CyclePos;
	
	// Common voice parameters
	Spiral::Core::SetProperty m_Type;
	Spiral::Core::SignedProperty m_Octave;
	Spiral::Core::FloatProperty m_FineFreq;
	Spiral::Core::FloatProperty m_ModAmount;
		
	Sample m_Table[NUM_TABLES];
	int    m_TableLength;
	
	friend Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, WaveTablePlugin &o);
	friend Spiral::Core::Description &Describe(Spiral::Core::Description &d, WaveTablePlugin &o);
};

Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, WaveTablePlugin &o);
Spiral::Core::Description &Describe(Spiral::Core::Description &d, WaveTablePlugin &o);

#endif
