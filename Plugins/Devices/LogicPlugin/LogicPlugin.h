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

#ifndef PLUGIN
#define PLUGIN

class LogicPlugin : public Spiral::Core::Device
{
public:
 	LogicPlugin();
	virtual ~LogicPlugin();

	virtual PluginInfo& Initialise(const HostInfo *Host);
	virtual void 		Execute();
	virtual void 		ExecuteCommands();
	virtual void	    Describe(Spiral::Core::Description &d);
	virtual void	    Apply(Spiral::Core::Description::Reader &r);

	enum OperatorType{NONE,AND,OR,NOT,NAND,NOR,XOR,XNOR};

	OperatorType GetOperator() { return (OperatorType)AsUnsigned(m_Operator); }
        enum GUICommands {NOCMD, SETINPUTS};
        int GetInputs (void) { return AsUnsigned(m_Inputs); }

private:
	Spiral::Core::SetProperty m_Operator;
        Spiral::Core::UnsignedProperty m_Inputs;
        void SetInputs (int n);
        void CreatePorts (int n = 2, bool AddPorts = false);
};

#endif
