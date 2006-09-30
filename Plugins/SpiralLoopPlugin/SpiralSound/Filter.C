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

#include "Filter.h"
#include "Sample.h"

static const float MAX_RES = 10;
static const float MIN_RES = 1;
static const float MAX_CUTOFF = 3000;
static const float MIN_CUTOFF = 500;

Filter::Filter() :
fs(44100),     // Sampling frequency 
fc(-1),             // Filter cutoff 
Q(1),               // Resonance 
m_LastFC(0),
m_LastQ(1),
k(1.0),              // Set overall filter gain
m_RevCutoffMod(false),
m_RevResonanceMod(false),
m_FilterBypass(true),
m_CutoffModBuf(NULL),
m_ResonanceModBuf(NULL)
{
 	SetupCoeffs();


	iir = new FILTER[1];

	for (int n=0; n<1; n++)
	{
    	iir[n].length = FILTER_SECTIONS;         	
    	iir[n].coef = (float *) calloc(4 * iir[n].length + 1 , sizeof(float)); 
	
    	if (!iir[n].coef) 
    	{ 
    	     printf("Unable to allocate coef array, exiting\n"); 
    	     exit(1); 
    	} 		
	}
}

Filter::~Filter()
{
	delete[] iir;
}

void Filter::SetupCoeffs()
{
	ProtoCoef[0].a0 = 1.0; 
    ProtoCoef[0].a1 = 0; 
    ProtoCoef[0].a2 = 0; 
    ProtoCoef[0].b0 = 1.0; 
    ProtoCoef[0].b1 = 0.765367; 
    ProtoCoef[0].b2 = 1.0; 

    ProtoCoef[1].a0 = 1.0; 
    ProtoCoef[1].a1 = 0; 
    ProtoCoef[1].a2 = 0; 
    ProtoCoef[1].b0 = 1.0; 
    ProtoCoef[1].b1 = 1.847759; 
    ProtoCoef[1].b2 = 1.0; 
}

void Filter::GetOutput(int V, Sample &data)
{   
   float Cutoff;
   float Resonance;
   
   if (m_FilterBypass || fc<0) return;
   
   for (int n=0; n<data.GetLength(); n++)
	 {
  	 coef = iir[V].coef + 1;     /* Skip k, or gain */ 
   	 k=0.25;
	
	 Cutoff = fc;
	 Resonance = Q; 
	 
	 Cutoff/=2;
	 
	 if (Resonance>MAX_RES) Resonance=MAX_RES; 
	 if (Cutoff>MAX_CUTOFF) Cutoff=MAX_CUTOFF; 
	 if (Resonance<MIN_RES) Resonance=MIN_RES; 
	 if (Cutoff<MIN_CUTOFF) Cutoff=MIN_CUTOFF; 
	 
	 if (n%10==0)
	 {
	 	// if different from last time
	 	//if (m_LastQ!=Q || m_LastFC!=fc)
		{				
		 	for (nInd = 0; nInd < iir[V].length; nInd++) 
    	   	{
		        a2 = ProtoCoef[nInd].a2; 
	
	            a0 = ProtoCoef[nInd].a0; 
	            a1 = ProtoCoef[nInd].a1; 
	          
	            b0 = ProtoCoef[nInd].b0; 
	            b1 = ProtoCoef[nInd].b1 / Resonance;      
	            b2 = ProtoCoef[nInd].b2; 	
	            szxform(&a0, &a1, &a2, &b0, &b1, &b2, Cutoff*(Cutoff/1000.0f), fs, &k, coef); 
    	        coef += 4;                       
			} 

        	iir[V].coef[0] = k; 
			
			m_LastQ=Q;
			m_LastFC=fc;
		
		}
	 }
	  	 	 
	 data.Set(n,iir_filter(data[n],&iir[V]));	 
     }
 
}

void Filter::ModulateCutoff(Sample *data)
{
	m_CutoffModBuf=data;
}

void Filter::ModulateResonance(Sample *data)
{
	m_ResonanceModBuf=data;
}

void Filter::Reset()
{
	SetupCoeffs();

	/*for (int n=0; n<FILTER_SECTIONS+1; n++)
	{
		iir.coef[n]=0;
		iir.history[n]=0;
	}*/
}

void Filter::Randomise()
{
	//Q=RandFloat(1.0f,10.0f);
	//fc=RandFloat(10.0f,10000.0f);
	//m_RevCutoffMod=(RandFloat()>0.5f);
	//m_RevResonanceMod=(RandFloat()>0.5f);
}

istream &operator>>(istream &s, Filter &o)
{
	s>>o.fc>>o.Q>>o.m_RevCutoffMod>>o.m_RevResonanceMod;
	return s;
}

ostream &operator<<(ostream &s, Filter &o)
{
	s<<o.fc<<" "<<o.Q<<" "<<o.m_RevCutoffMod<<" "<<o.m_RevResonanceMod<<" ";
	return s;
}

