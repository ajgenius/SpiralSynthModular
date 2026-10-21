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
#include <string.h>

#ifndef ControllerPLUGIN
#define ControllerPLUGIN

static const int MAX_CHANNELS = 99;

class ControllerPlugin : public Spiral::Core::Device
{
public:
 	ControllerPlugin();
	virtual ~ControllerPlugin();
	
	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void ExecuteCommands();
	virtual void Describe(Spiral::Core::Description &d);
	virtual void Apply(Spiral::Core::Description::Reader &r);
	virtual void Migrate(Spiral::Core::Description &d);

	std::string GetName(int n) { return ((Spiral::Core::StringProperty *)m_Names[n])->Value; }
	int    GetNum()       { return AsUnsigned(m_NumChannels); }
	float  GetVal(int n)  { return ((Spiral::Core::FloatProperty *)m_ChannelVal[n])->Value.AsFloat; }
	float  GetMin(int n)  { return ((Spiral::Core::FloatProperty *)m_MinVal[n])->Value.AsFloat; }
	float  GetMax(int n)  { return ((Spiral::Core::FloatProperty *)m_MaxVal[n])->Value.AsFloat; }
	Spiral::Core::StringProperty &Name(int n) { return *(Spiral::Core::StringProperty *)m_Names[n]; }
	Spiral::Core::FloatProperty &Val(int n) { return *(Spiral::Core::FloatProperty *)m_ChannelVal[n]; }
	Spiral::Core::FloatProperty &Min(int n) { return *(Spiral::Core::FloatProperty *)m_MinVal[n]; }
	Spiral::Core::FloatProperty &Max(int n) { return *(Spiral::Core::FloatProperty *)m_MaxVal[n]; }

private:
	char  m_NameChannel[MAX_CHANNELS][256];
	void SyncNames();

	void SetNum(int n);
	void SetName(int n, const char* name)
		{ ((Spiral::Core::StringProperty *)m_Names[n])->Value = name;
		  strncpy(m_NameChannel[n], name, 255); m_NameChannel[n][255] = 0; };
	void SetChannel(int n, float value) { ((Spiral::Core::FloatProperty *)m_ChannelVal[n])->Value.AsFloat = value; };
	void SetMax(int n, float max)       { ((Spiral::Core::FloatProperty *)m_MaxVal[n])->Value.AsFloat = max; };
	void SetMin(int n, float min)       { ((Spiral::Core::FloatProperty *)m_MinVal[n])->Value.AsFloat = min; };

	void SetAll(int n, const char *name, float max, float value, float min)
		{ SetName(n, name);
		  SetMax(n, max);
		  SetChannel(n, value);
		  SetMin(n, min);
		};
	void Clear();

	Spiral::Core::UnsignedProperty m_NumChannels;
    Spiral::Core::VectorProperty m_ChannelVal;
    Spiral::Core::VectorProperty m_Names;
    Spiral::Core::VectorProperty m_MinVal;
    Spiral::Core::VectorProperty m_MaxVal;
};

#endif
