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

#ifndef SEQSELECTORPlugin
#define SEQSELECTORPlugin

static const int NUM_VALUES = 8;

class SeqSelectorPlugin : public Spiral::Core::Device
{
public:
 	SeqSelectorPlugin();
	virtual ~SeqSelectorPlugin();
	
	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void Reset();
	virtual void ExecuteCommands();
	virtual void Describe(Spiral::Core::Description &d);
	virtual void Apply(Spiral::Core::Description::Reader &r);
		
	int  GetBegin()      { return AsSigned(m_Begin); }
	int  GetEnd()        { return AsSigned(m_End); }
			
	enum GUICommands{NONE,SET_BEGIN,SET_END,RANGE,ADD_LINE,REM_LINE,SET_VAL};
	
	struct GUIArgs
	{
		int Num;
		int Line;
		int Val;
	};
	
	struct Line
	{
		std::vector<Spiral::Core::SignedProperty*> Value;
	};
	
	std::vector<Line> m_Lines;
	
private:

	void UpdateLineCount(unsigned int num);

	GUIArgs m_GUIArgs;
	
	Spiral::Core::UnsignedProperty m_NumLines, m_Position;
	Spiral::Core::SignedProperty m_Begin, m_End;
	Spiral::Core::ToggleProperty m_UseRange;
	bool m_Triggered;
};

#endif
