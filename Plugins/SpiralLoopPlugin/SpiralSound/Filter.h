/*  SpiralSynth
 *  Copyleft (C) 2000 David Griffiths <dave@pawfal.org>
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

#include <math.h>
#include "NoteTable.h"
#include "iir_filter.h"

#ifndef _FILTER
#define _FILTER


class Filter
{
public:
	Filter();
	~Filter();
	
	void GetOutput(int V, Sample &data);
	void Reset();
	void SetupCoeffs();

	void SetCutoff(double s) {fc=s;}
	void SetResonance(double s) {Q=s;}
	void SetRevCutoffMod(bool s) {m_RevCutoffMod=s;}
	void SetRevResonanceMod(bool s) {m_RevResonanceMod=s;}
	void FilterBypass(bool s) {m_FilterBypass=s;}
	double GetCutoff() {return fc;}
	double GetResonance() {return Q;}
	bool GetRevCutoffMod() {return m_RevCutoffMod;}
	bool GetRevResonanceMod() {return m_RevResonanceMod;}
	
	void ModulateCutoff(Sample *data);
	void ModulateResonance(Sample *data);
	void Randomise();
	
private:
	
	// Voice specifics
	FILTER   *iir; 
   	
	// Voice independant
	float    *coef; 
    double   fs, fc;     // Sampling frequency, cutoff frequency  
    double   Q;          // Resonance > 1.0 < 1000  
	double   m_LastFC;
	double   m_LastQ;
    unsigned nInd; 
    double   a0, a1, a2, b0, b1, b2; 
    double   k;          // overall gain factor 
	bool	 m_RevCutoffMod;
	bool	 m_RevResonanceMod;
	bool     m_FilterBypass;
	
	Sample *m_CutoffModBuf;
	Sample *m_ResonanceModBuf;
	
	friend istream &operator>>(istream &s, Filter &o);
	friend ostream &operator<<(ostream &s, Filter &o);
};

istream &operator>>(istream &s, Filter &o);
ostream &operator<<(ostream &s, Filter &o);

#endif
