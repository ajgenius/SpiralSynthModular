// Copyright (C) 2004 David Griffiths <dave@pawfal.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <iostream>
#include <stdlib.h>
#include "Tuna.h"

static const float MAX_LUT_FREQ = 24000.0; // (maximum audible freq (apparently))

Tuna::Tuna() :
m_Buffer(NULL),
m_Root(440.0),
m_Highest(0)
{
	// start off with boring equal temp
	m_NumNotes=12;
	m_Scale.push_back(1+100/1200.0);
	m_Scale.push_back(1+200/1200.0);
	m_Scale.push_back(1+300/1200.0);
	m_Scale.push_back(1+400/1200.0);
	m_Scale.push_back(1+500/1200.0);
	m_Scale.push_back(1+600/1200.0);
	m_Scale.push_back(1+700/1200.0);
	m_Scale.push_back(1+800/1200.0);
	m_Scale.push_back(1+900/1200.0);
	m_Scale.push_back(1+1000/1200.0);
	m_Scale.push_back(1+1100/1200.0);
	m_Scale.push_back(2);
	
	CalculateNoteLUT();
}

Tuna::~Tuna() 
{
}

bool Tuna::Open(const string &filename) 
{
	FILE *file = fopen(filename.c_str(),"r");
	if (!file)              
	{
    	cerr<<"Tuna::Open: could not open "<<filename<<endl;
    	return false;
	}

	fseek(file,0,SEEK_END);
	m_Size=ftell(file);
	fseek(file,0,SEEK_SET);
	m_Buffer = new char[m_Size];

	if (fread(m_Buffer,1,m_Size,file)!=m_Size)
	{
    	cerr<<"Tuna::Open: error loading "<<filename<<endl;
    	return false;
	}

	Parse();
	fclose(file);
	return true;
}

void Tuna::Parse()
{
	if (!m_Buffer) return;

	m_Scale.clear();
	//m_Scale.push_back(1); // add implicit 1/1 note

	unsigned int position = 0;
	bool readcomment = false;
	bool readnumnotes = false;

	while (position<m_Size)
	{
    	if (m_Buffer[position]=='!') // comment line
    	{
            position = SkipToNextLine(position);
    	}
    	else if (!readcomment) // description comes first
    	{
            position = ReadLine(m_Description,position);
            readcomment=true;
    	}
    	else
    	{
			if (!readnumnotes) // next is number of notes
			{
    			string numnotes;
    			position = ReadLine(numnotes,position);
    			readnumnotes=true;
    			m_NumNotes = (int)atof(numnotes.c_str());
			}
			else
			{
    			string noteinfo;
    			position = ReadLine(noteinfo,position);

    			if (noteinfo.find_first_of('/')!=string::npos)
    			{
            			// we got a ratio
					int slashpos = noteinfo.find_first_of('/');
					float numerator = atof((noteinfo.substr(0,slashpos)).c_str());
					float denominator = atof((noteinfo.substr(slashpos+1,noteinfo.size())).c_str());
					if (denominator!=0)
					{
    					float result = 1+log(numerator/denominator)/log(2.0);                                   
    					if (finite(result)) m_Scale.push_back(result);                  
    					else cerr<<"Tuna::Parse: error in scala file ["<<noteinfo<<"]"<<endl;
					}
					else cerr<<"Tuna::Parse: denominator parsed as zero ["<<noteinfo<<"]"<<endl;
    			} 
    			else if (noteinfo.find_first_of('.')!=string::npos)
    			{
            		// we got a cents value
            		float result = 1+atof(noteinfo.c_str())/1200.0;
            		if (finite(result)) m_Scale.push_back(result);                  
            		else cerr<<"Tuna::Parse: error in scala file ["<<noteinfo<<"]"<<endl;
    			}
    			else
    			{
            		// we got an integer
            		float result = atof(noteinfo.c_str());                                     
            		if (finite(result)) m_Scale.push_back(result);                  
            		else cerr<<"Tuna::Parse: error in scala file ["<<noteinfo<<"]"<<endl;
    			}
			}
    	}
	}       

	CalculateNoteLUT();
}

void Tuna::Print()
{
	for (vector<float>::iterator i=m_Scale.begin(); i!=m_Scale.end(); i++)
	{
    	cerr<<*i<<endl;
	}
	
	cerr<<"----------------"<<endl;
	
	for (vector<float>::iterator i=m_NoteLUT.begin(); i!=m_NoteLUT.end(); i++)
	{
    	cerr<<*i<<endl;
	}
	
}

int Tuna::SkipToNextLine(unsigned int position)
{
    while(position<m_Size && m_Buffer[position]!='\n') position++;
    return ++position;
}

int Tuna::ReadLine(string &output, unsigned int position)
{
    output="";
    while(position<m_Size && m_Buffer[position]!='\n') output.push_back(m_Buffer[position++]);
    return ++position;
}

float Tuna::GetQuantised(float frequency)
{
    float result = 0;
    SnapFrequency(frequency, result);
    return result;
}

int Tuna::SnapFrequency(float in, float &out, bool usefilter)
{
	if (m_NoteLUT.size()==0) 
	{
		out=in;
		return 0;
	}
	
	// gone too high?
	if (in>m_Highest)
	{ 
		out=m_Highest;
		return m_NoteLUT.size();
	}
	 
	// or too low?
	if (in<=*m_NoteLUT.begin())
	{
		out=*m_NoteLUT.begin();
		return 1;
	}
	 
    // snap frequency from 'in' to the nearest specified by the LUT
    return FindClosest(in, 0, m_NoteLUT.size(), out, usefilter);
}

// binary chop to the closest LUT freq
int Tuna::FindClosest(float in, unsigned int low, int unsigned high, float &out, bool usefilter)
{
	unsigned int mid = (high+low)/2;

	// if the value is somewhere between the next smallest, and next biggest        
	if (in>=m_NoteLUT[mid-1] && in<=m_NoteLUT[mid+1]) 
	{
    	float closest = FLT_MAX;
    	int index = 0;

    	// need to search a whole octave for the right note, 
    	// as some or all may be disabled by the filter...
    	unsigned int halfoctave = m_NumNotes/2;
    	for (unsigned int n=mid-halfoctave; n<mid+halfoctave; n++)
    	{
			if (n>=0 && n<m_NoteLUT.size())
    		{
            	float dist = fabs(m_NoteLUT[n]-in);
            	if (dist<closest)
            	{
                    closest=dist;
                    out=m_NoteLUT[n];
                    index=n;
            	}
    		}			
    	}               
    	// return closest
    	return index;           
	}

	// chop!
	if (in>m_NoteLUT[mid]) 
	{
    	//cerr<<"going high "<<mid<<" "<<high<<endl;
		if (high==mid) 
		{
			out=m_NoteLUT[mid];
			return 0;
		}
		else 
		{
			return FindClosest(in, mid, high, out, usefilter);
		}
	}
	else 
	{
    	//cerr<<"going low "<<low<<" "<<mid<<endl;
    	if (low==mid) 
		{
			out=m_NoteLUT[mid];
			return 0;
		}
		else
		{
			return FindClosest(in, low, mid, out, usefilter);
		}
	}
}

// tried to calculate this properly, but failed and used a LUT :)
void Tuna::CalculateNoteLUT()
{
	if (m_Root<=0) return;

	m_NoteLUT.clear();

	float freq=m_Root;
	
	while (freq/16<MAX_LUT_FREQ)
	{               
    	for (vector<float>::iterator i=m_Scale.begin(); i!=m_Scale.end(); i++)
    	{
			m_Highest = pow(2.0f,*i)*freq;
			m_NoteLUT.push_back(m_Highest/16.0f);
    	}
    	freq*=2;
	}
	
}

float Tuna::GetNote(unsigned int index) 
{ 
	//int ANote = 69;
	//float AFreq = 440.0;
	//return (pow(2.0,index/12.0)*AFreq)/4.0f; //12tET
    
	if (index>=m_Scale.size()) index%=m_NoteLUT.size();
	return m_NoteLUT[index]; 
}

/*
int Tuna::SnapFrequency(float in, float &out, bool usefilter)
{
        float normalised = in/m_Root;
        
        int octave=0;
        if (normalised>=1) octave = (int)floor(normalised);
        else octave = -(int)floor(1/normalised);
        
        float scaletarget = 1+(normalised-abs(octave));
        out = 0;
        
        // quantise fractional part
        float closest = FLT_MAX;
        int note=-1;
        for (int i=0; i<=m_Scale.size(); i++)
        {
                float distance = fabs(scaletarget-m_Scale[i]);
                if (distance<closest)
                {
                        closest=distance;
                        out=m_Scale[i];
                        note=i;
                }
                
        }
        
        // remove scale octave thing (scales go from 1->2)
        out-=1;
        
        // stick whole number back on for octave
        cerr<<out<<" "<<octave<<endl;
        if (octave>=0) out+=octave;
        else out/=(float)abs(octave);

        // calculate back into frequency                
        out*=m_Root;
        
        cerr<<note<<" "<<out<<" "<<octave<<" "<<scaletarget<<endl;
        
        return note;
}
*/

