/*  JoystickPlugin
 *  Copyleft (C) 2002 William Bland <wjb@abstractnonsense.com>
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
#include <linux/joystick.h>
#include <FL/Fl_Pixmap.H>

#ifndef JoystickPLUGIN
#define JoystickPLUGIN

#define JP_NUMBER_OF_AXES 3
#define JP_NUMBER_OF_BUTTONS 3

class LowLevelJoystick
{
public:
	static LowLevelJoystick *Get()       { if(!m_Singleton) m_Singleton=new LowLevelJoystick; return m_Singleton; }
	static void PackUpAndGoHome() { if(m_Singleton)  { delete m_Singleton; m_Singleton=NULL; } }
	~LowLevelJoystick(); 
	
	void    Read();
	void    OpenRead();
	void    Close();
	float   GetJoystickAxis( int );
	float   GetJoystickButton( int );

	bool    m_invert_axis[JP_NUMBER_OF_AXES];
	
private:
	static LowLevelJoystick* m_Singleton;

 	LowLevelJoystick();
	float   m_button[JP_NUMBER_OF_BUTTONS];
	float   m_axis[JP_NUMBER_OF_AXES];

	int     m_Joyfd;
	bool    m_InputOk;
};


class JoystickPlugin : public SpiralPlugin
{
public:
 	JoystickPlugin();
	virtual ~JoystickPlugin();

	virtual PluginInfo& Initialise(const HostInfo *Host);
	virtual SpiralGUIType*  CreateGUI();
	virtual void 		Execute();
	virtual void	    StreamOut(std::ostream &s) {}
	virtual void	    StreamIn(std::istream &s)  {}
	
	// has to be defined in the plugin	
	virtual void UpdateGUI() { Fl::check(); }
	
private:
	static int m_RefCount;
	static int m_NoExecuted;
	float      m_last_axis[JP_NUMBER_OF_AXES];
};

#endif
