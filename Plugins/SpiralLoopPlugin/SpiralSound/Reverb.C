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

// Stereo module

#include "Reverb.h"

static const int NUM_DELAYS = 5;
static const float MAX_DELAYTIME = 0.2;
static const float MIN_DELAYTIME = 0.02;

float RandFloat(float min, float max)
{
	return min+(rand()%1000/1000.0*(max-min));
}

Reverb::Reverb() :
m_Time(0),
m_Feedback(0.2),
m_Randomness(0.01),
m_Bypass(true)
{
	// set up the delay vec
	for (int i=0; i<NUM_DELAYS; i++)
	{
		Delay *pDelay = new Delay;
		pDelay->SetLeftDelay(RandFloat(MIN_DELAYTIME,MAX_DELAYTIME));
		pDelay->SetRightDelay(RandFloat(MIN_DELAYTIME,MAX_DELAYTIME));
		pDelay->SetBypass(false);
		m_DelayVec.push_back(pDelay);
	}
}

Reverb::~Reverb()
{
	for (vector<Delay*>::iterator i=m_DelayVec.begin();
		 i!=m_DelayVec.end(); i++)
	{
		delete *i;
	}
	
	m_DelayVec.clear();
}

void Reverb::SetTime(float s)
{
	m_Time=s;
	float t=0;
	int count=0;
	for (vector<Delay*>::iterator i=m_DelayVec.begin();
		 i!=m_DelayVec.end(); i++)
	{
		t=count/(float)NUM_DELAYS;
		(*i)->SetLeftDelay(t*m_Time+RandFloat(-m_Randomness,m_Randomness));
		(*i)->SetRightDelay(t*m_Time+RandFloat(-m_Randomness,m_Randomness));
		count++;
	}
}

void Reverb::SetFeedback(float s)
{
	m_Feedback=s;
	for (vector<Delay*>::iterator i=m_DelayVec.begin();
		 i!=m_DelayVec.end(); i++)
	{
		(*i)->SetFeedback(s);
	}
}

void Reverb::SetNumChannels(int s)
{
	for (vector<Delay*>::iterator i=m_DelayVec.begin();
		 i!=m_DelayVec.end(); i++)
	{
		(*i)->SetNumChannels(s);
	}
}

void Reverb::GetOutput(Sample &data)
{
	if (m_Time==0 || m_Bypass) return;
	
	for (vector<Delay*>::iterator i=m_DelayVec.begin();
		 i!=m_DelayVec.end(); i++)
	{
		(*i)->GetOutput(data);
	}
}

istream &operator>>(istream &s, Reverb &o)
{
	s>>o.m_Time>>o.m_Feedback>>o.m_Randomness>>o.m_Bypass;
	return s;
}

ostream &operator<<(ostream &s, Reverb &o)
{
	s<<o.m_Time<<" "<<o.m_Feedback<<" "<<o.m_Randomness<<" "<<o.m_Bypass<<" ";
	return s;
}
