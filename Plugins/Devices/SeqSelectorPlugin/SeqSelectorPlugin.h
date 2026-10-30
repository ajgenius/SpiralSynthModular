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
	virtual void Migrate(Spiral::Core::Description &d);
		
	int  GetBegin()      { return AsSigned(m_Begin); }
	int  GetEnd()        { return AsSigned(m_End); }
	bool GetUseRange()   { return AsBoolean(m_UseRange); }
			
	enum GUICommands{NONE,SET_BEGIN,SET_END,RANGE,ADD_LINE,REM_LINE,SET_VAL};
	
	struct GUIArgs
	{
		int Num;
		int Line;
		int Val;
	};
	
	struct Line
	{
		int Value[NUM_VALUES];
	};
	
	Spiral::Core::VectorProperty m_Lines;

	Spiral::Core::VectorProperty *NewLine();
	Spiral::Core::SignedProperty &Cell(int line, int n)
	{ return *(Spiral::Core::SignedProperty *)(*(Spiral::Core::VectorProperty *)m_Lines[line])[n]; }
	int  LineValue(int line, int n)
	{ return ((Spiral::Core::SignedProperty *)(*(Spiral::Core::VectorProperty *)m_Lines[line])[n])->Value.AsSigned; }
	void SetLineValue(int line, int n, int v)
	{ ((Spiral::Core::SignedProperty *)(*(Spiral::Core::VectorProperty *)m_Lines[line])[n])->Value.AsSigned = v; }
	
private:

	GUIArgs m_GUIArgs;
	
	unsigned int m_Pos;
	Spiral::Core::SignedProperty m_Begin;
	Spiral::Core::SignedProperty m_End;
	Spiral::Core::ToggleProperty m_UseRange;
	bool m_Triggered;
};

#endif
