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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// not realtime - contains mallocs

using namespace std;

#ifndef TUNA_INCLUDED
#define TUNA_INCLUDED

class Tuna
{
public:
    Tuna();
    ~Tuna();

    // read a scala (.scl) scale file
    bool Open(const string &filename);
    string GetDescription() { return m_Description; }
    float GetQuantised(float frequency);
    void Print();
    void SetRoot(float s) { if (s>0) m_Root=s; CalculateNoteLUT(); }
    float GetNote(unsigned int index);
	               
private:

    void Parse();
    int SkipToNextLine(unsigned int position);
    int ReadLine(string &output, unsigned int position);
    int SnapFrequency(float in, float &out, bool usefilter=true);
    void CalculateNoteLUT();
    int FindClosest(float in, unsigned int low, unsigned int high, float &out, bool usefilter);

    char *m_Buffer;
    unsigned int m_Size;
    string m_Description;
    unsigned int m_NumNotes;
    vector<float> m_Scale;
    float m_Root;
    vector<float> m_NoteLUT;
	float m_Highest;
};

#endif
