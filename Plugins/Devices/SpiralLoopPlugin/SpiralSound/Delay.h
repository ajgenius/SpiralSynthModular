/*  SpiralSound
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

// Stereo module, different delay times for left and right, due to reverb
// functionality.

#include <math.h>

#ifndef DELAY
#define DELAY

class Delay
{
public:
	Delay();
	~Delay();

	void  SetDelay(float s) {m_LeftDelay=s; m_RightDelay=s;}	
	void  SetRightDelay(float s) {m_RightDelay=s;}	
	void  SetLeftDelay(float s) {m_LeftDelay=s;}	
	void  SetNumChannels(int s) {m_Channels=s;}
	void  SetFeedback(float s) {m_Feedback=s;}	
	void  SetBypass(bool s) {m_Bypass=s;}
	float GetDelay() {return GetLeftDelay();}	
	float GetLeftDelay() {return m_LeftDelay;}	
	float GetRightDelay() {return m_RightDelay;}	
	float GetFeedback() {return m_Feedback;}	
	float GetBypass() {return m_Bypass;}	
	void  GetOutput(Sample &data);
	void  Randomise();

private:

	Sample m_LeftBuffer;
	Sample m_RightBuffer;
	long   m_LeftWrite;
	long   m_RightWrite;
	float  m_LeftDelay;
	float  m_RightDelay;
	float  m_Feedback;
	int    m_Channels;
	bool   m_Bypass;
	
	friend istream &operator>>(istream &s, Delay &o);
	friend ostream &operator<<(ostream &s, Delay &o);
};

istream &operator>>(istream &s, Delay &o);
ostream &operator<<(ostream &s, Delay &o);

#endif
