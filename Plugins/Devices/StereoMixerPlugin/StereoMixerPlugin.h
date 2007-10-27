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
#include <vector>

#ifndef StereoMixerPLUGIN
#define StereoMixerPLUGIN

static const int NUM_CHANNELS = 4;

class StereoMixerPlugin : public Spiral::Core::Device
{
public:
 	StereoMixerPlugin();
	virtual ~StereoMixerPlugin();
	
	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void ExecuteCommands();
	virtual void Describe(Spiral::Core::Description &d);
	virtual void Apply(Spiral::Core::Description::Reader &r);
		
	float GetChannel(int n) { return m_Volume[n]->Value.AsFloat; }
	float GetPan(int n) { return m_Pan[n]->Value.AsFloat; }
	
	enum GUICommands {NONE,SETCH,SETPAN};
	struct GUIArgs
	{
		int Num;
		float Value;
	};
	
private:
	GUIArgs m_GUIArgs;

	void  SetChannel(int n, float s) { m_Volume[n]->Value.AsFloat=s; }
	void  SetPan(int n, float s) { m_Pan[n]->Value.AsFloat=s; }
	void  EnsureChannelProperties();

	Spiral::Core::UnsignedProperty m_NumChannels;
	std::vector<Spiral::Core::FloatProperty*> m_Volume;
	std::vector<Spiral::Core::FloatProperty*> m_Pan;
};

#endif
