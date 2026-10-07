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

// Stereo module

#include "Delay.h"
#include "Sample.h"
using spiralcore::Sample;
using spiralcore::Linear;
using spiralcore::feq;

static const int DELAYBUFFERSIZE=44100;

Delay::Delay() :
m_LeftBuffer(DELAYBUFFERSIZE),
m_RightBuffer(DELAYBUFFERSIZE),
m_LeftWrite(0),
m_RightWrite(0),
m_LeftDelay(0.5),
m_RightDelay(0.5),
m_Feedback(0.5),
m_Channels(1),
m_Bypass(true)
{
}

Delay::~Delay()
{
}

void Delay::GetOutput(Sample &data)
{
	if (m_Bypass) return;
	
	int n=0;
	long RLength=static_cast<long>(m_RightDelay*(DELAYBUFFERSIZE-1));
	long LLength=static_cast<long>(m_LeftDelay*(DELAYBUFFERSIZE-1));
	long temp=0;
	
	while (n<data.GetLength()*m_Channels)
	{
		// left side
		m_LeftBuffer.Set(m_LeftWrite,data[n]+m_LeftBuffer[(int)m_LeftWrite]*m_Feedback);
		data.Set(n,m_LeftBuffer[(int)m_LeftWrite]);
		n++;

		if (m_Channels==2)
		{
			// right side
			m_RightBuffer.Set(m_RightWrite,data[n]+m_RightBuffer[(int)m_RightWrite]*m_Feedback);
			data.Set(n,m_RightBuffer[(int)m_RightWrite]);
			n++;
		}
		
		m_LeftWrite++;
		if (m_LeftWrite>LLength) m_LeftWrite=0;
		m_RightWrite++;
		if (m_RightWrite>RLength) m_RightWrite=0;
	}
}

void Delay::Randomise()
{
	//m_RightDelay=RandFloat();
	//m_LeftDelay=m_RightDelay;
	//m_Feedback=RandFloat();
}


istream &operator>>(istream &s, Delay &o)
{
	s>>o.m_LeftDelay>>o.m_Feedback>>o.m_Bypass;
	return s;
}

ostream &operator<<(ostream &s, Delay &o)
{
	s<<o.m_LeftDelay<<" "<<o.m_Feedback<<" "<<o.m_Bypass<<" ";
	o.m_RightDelay=o.m_LeftDelay;
	return s;
}
