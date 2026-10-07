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
#include <math.h>
#include "Delay.h"
#include <vector>

#ifndef REVERB
#define REVERB

class Reverb
{
public:
	Reverb();
	~Reverb();

	void  SetTime(float s);
	void  SetRandomness(float s) {m_Randomness=s; SetTime(m_Time);}
	void  SetFeedback(float s);
	void  SetNumChannels(int s);
	void  SetBypass(bool s) {m_Bypass=s;}
	float GetTime() {return m_Time;}
	float GetFeedback() {return m_Feedback;}
	float GetRandomness() {return m_Randomness;}
	bool  IsBypassed() {return m_Bypass;}
	void  GetOutput(Sample &data);
	void  Randomise();

private:

	float m_Time;
	float m_Feedback;
	float m_Randomness;
	bool  m_Bypass;
	
	vector<Delay*> m_DelayVec;
	
	friend istream &operator>>(istream &s, Reverb &o);
	friend ostream &operator<<(ostream &s, Reverb &o);
};

istream &operator>>(istream &s, Reverb &o);
ostream &operator<<(ostream &s, Reverb &o);

#endif
