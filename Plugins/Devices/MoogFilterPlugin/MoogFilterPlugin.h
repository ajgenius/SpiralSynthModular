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

#ifndef MOOGFILTERPLUGIN
#define MOOGFILTERPLUGIN

class MoogFilterPlugin : public Spiral::Core::Device
{
public:
 	MoogFilterPlugin();
	virtual ~MoogFilterPlugin();
	
	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void Reset();

	
	float GetCutoff()    { return AsFloat(Cutoff); }
	float GetResonance() { return AsFloat(Resonance); }
	unsigned GetMode()   { return AsUnsigned(Mode); }
	
	void Clear();
	void Randomise();

private:
	void ExecuteLegacy();
	void ExecuteTPT();

	Spiral::Core::FloatProperty Cutoff, Resonance;
	Spiral::Core::UnsignedProperty Mode;
	
	float fs, fc;
	float f,p,q;
	float b0,b1,b2,b3,b4;
	float t1,t2;
	float tpt_s1, tpt_s2, tpt_s3, tpt_s4, tpt_y4;
	
	float in1,in2,in3,in4,out1,out2,out3,out4;
	
	friend Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, MoogFilterPlugin &o);
	friend Spiral::Core::Description &Describe(Spiral::Core::Description &d, MoogFilterPlugin &o);
};
Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, MoogFilterPlugin &o);
Spiral::Core::Description &Describe(Spiral::Core::Description &d, MoogFilterPlugin &o);

#endif
