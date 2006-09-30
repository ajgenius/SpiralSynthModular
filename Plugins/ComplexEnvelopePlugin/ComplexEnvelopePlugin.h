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

#include "SpiralPlugin.h"
#include "Bezier.h"
#include <FL/Fl_Pixmap.H>

#ifndef ComplexEnvelopePLUGIN
#define ComplexEnvelopePLUGIN

static const int NUM_CHANNELS = 4;

class ComplexEnvelopePlugin : public SpiralPlugin
{
public:
 	ComplexEnvelopePlugin();
	virtual ~ComplexEnvelopePlugin();
	
	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual SpiralGUIType *CreateGUI();
	virtual void Execute();
	virtual void StreamOut(ostream &s);
	virtual void StreamIn(istream &s);
	
	// has to be defined in the plugin	
	virtual void UpdateGUI() { Fl::check(); }
	void CVListToEnvelope(const vector<Vec2> &Lines);
	
	void SetLength(float s) { m_SampleTime=s; }
	void SetBezier(bool s)  { m_Bezier=s; }
	
private:
	
	float  m_Triggered; // should scale envelope
	int    m_Position;
	bool   m_Bezier;
	Sample m_EnvSample;
	float  m_SampleTime;

	vector<Vec2> m_CVVec;
	
	friend istream &operator>>(istream &s, ComplexEnvelopePlugin &o);
	friend ostream &operator<<(ostream &s, ComplexEnvelopePlugin &o);
};
istream &operator>>(istream &s, ComplexEnvelopePlugin &o);
ostream &operator<<(ostream &s, ComplexEnvelopePlugin &o);

#endif
