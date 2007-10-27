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

	std::string GetName(int n) { return m_Name[n]->Value; }
	int    GetNum()       { return AsUnsigned(m_NumChannels); }
	float  GetVal(int n)  { return m_Value[n]->Value.AsFloat; }
	float  GetMin(int n)  { return m_Min[n]->Value.AsFloat; }
	float  GetMax(int n)  { return m_Max[n]->Value.AsFloat; }

	enum GUICommands{NONE,SETNUM,SETALL,SETNAME,SETMAX,SETCHANNEL,SETMIN};
	struct GUIArgs
	{
		int Number;
		float Value;
		float Min;
		float Max;
		char Name[256];
	};

private:
	GUIArgs m_GUIArgs;

	void SetNum(int n);
	void SetAll(int n, char *name, float max, float value, float min)
		{ m_Name[n]->Value         = name;
		  m_Max[n]->Value.AsFloat   = max;
		  m_Value[n]->Value.AsFloat = value;
		  m_Min[n]->Value.AsFloat   = min;
		};
	void SetName(int n, char* name)     { m_Name[n]->Value          = name;  };
	void SetChannel(int n, float value) { m_Value[n]->Value.AsFloat = value; };
	void SetMax(int n, float max)       { m_Max[n]->Value.AsFloat   = max;   };
	void SetMin(int n, float min)       { m_Min[n]->Value.AsFloat   = min;   };
	void Clear();
	void EnsureChannelProperties();

	Spiral::Core::UnsignedProperty m_NumChannels;
	std::vector<Spiral::Core::FloatProperty*> m_Value, m_Min, m_Max;
	std::vector<Spiral::Core::StringProperty*> m_Name;
};

#endif
