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
#include "iir_filter.h"

#ifndef FilterPLUGIN
#define FilterPLUGIN

static const int NUM_CHANNELS = 4;

class FilterPlugin : public Spiral::Core::Device
{
public:
 	FilterPlugin();
	virtual ~FilterPlugin();
	
	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
		
	virtual void Reset();
	virtual void Migrate(Spiral::Core::Description &d);
	void SetupCoeffs();

	double GetCutoff() {return AsFloat(fc);}
	double GetResonance() {return AsFloat(Q);}
	bool GetRevCutoffMod() {return AsBoolean(m_RevCutoffMod);}
	bool GetRevResonanceMod() {return AsBoolean(m_RevResonanceMod);}
	
private:
	// Voice specifics
	FILTER   iir; 
   	
	// Voice independant
	float    *coef; 
    float   fs;
    Spiral::Core::FloatProperty   fc;     // Sampling frequency, cutoff frequency  
    Spiral::Core::FloatProperty   Q;          // Resonance > 1.0 < 1000  
	float   m_LastFC;
	float   m_LastQ;
    unsigned nInd; 
    double   a0, a1, a2, b0, b1, b2; 
    double   k;          // overall gain factor 
	Spiral::Core::ToggleProperty m_RevCutoffMod;
	Spiral::Core::ToggleProperty m_RevResonanceMod;

	friend Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, FilterPlugin &o);
	friend Spiral::Core::Description &Describe(Spiral::Core::Description &d, FilterPlugin &o);
};
Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, FilterPlugin &o);
Spiral::Core::Description &Describe(Spiral::Core::Description &d, FilterPlugin &o);

#endif
