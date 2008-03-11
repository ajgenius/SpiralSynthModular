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

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <limits.h>

#include "JoystickPlugin.h"
#include "JoystickPluginGUI.h"
#include <Fl/Fl_Button.H>
#include "SpiralIcon.xpm"

using namespace std;

LowLevelJoystick* LowLevelJoystick::m_Singleton = NULL;
int JoystickPlugin::m_RefCount=0;
int JoystickPlugin::m_NoExecuted=0;

extern "C" {
SpiralPlugin* SpiralPlugin_CreateInstance()
{
  return new JoystickPlugin;
}

char** SpiralPlugin_GetIcon()
{	
  return SpiralIcon_xpm;
}

int SpiralPlugin_GetID()
{
  return 0x0070;
}

string SpiralPlugin_GetGroupName()
{
	return "SpiralSound";
}
}

///////////////////////////////////////////////////////

JoystickPlugin::JoystickPlugin() 
{
  m_RefCount++;
	
  m_PluginInfo.Name = "Joystick";
  m_PluginInfo.Width = 150;
  m_PluginInfo.Height = 60;
  m_PluginInfo.NumInputs = 0;
  m_PluginInfo.NumOutputs = JP_NUMBER_OF_AXES + JP_NUMBER_OF_BUTTONS;
  m_PluginInfo.PortTips.push_back("X-axis CV Out");
  m_PluginInfo.PortTips.push_back("Y-axis CV Out");
  m_PluginInfo.PortTips.push_back("Z-axis CV Out");
  m_PluginInfo.PortTips.push_back("Button 1 trigger Out");
  m_PluginInfo.PortTips.push_back("Button 2 trigger Out");
  m_PluginInfo.PortTips.push_back("Button 3 trigger Out");

  for( int i=0; i<JP_NUMBER_OF_AXES; i++ )
    {
      m_last_axis[i] = LowLevelJoystick::Get()->GetJoystickAxis( i );
      LowLevelJoystick::Get()->m_invert_axis[i] = 0;
    }
}

JoystickPlugin::~JoystickPlugin()
{
  m_RefCount--;
  if (m_RefCount==0)
    {
      LowLevelJoystick::PackUpAndGoHome();
    }
}

PluginInfo &JoystickPlugin::Initialise(const HostInfo *Host)
{	
  PluginInfo& Info= SpiralPlugin::Initialise(Host);
  return Info;
}

SpiralGUIType *JoystickPlugin::CreateGUI()
{
  m_GUI = new JoystickPluginGUI(m_PluginInfo.Width,
				m_PluginInfo.Height,
				this,m_HostInfo);
  m_GUI->hide();
  return m_GUI;
}


void JoystickPlugin::Execute()
{
  // Only Read() once per set of plugins
  m_NoExecuted++;
  if (m_NoExecuted==m_RefCount)
    {
      LowLevelJoystick::Get()->Read();		
      m_NoExecuted=0;
    }

  float stepsize[JP_NUMBER_OF_AXES];
  float newvalue[JP_NUMBER_OF_AXES];

  for( int i=0; i<JP_NUMBER_OF_AXES; i++ )
    {
      newvalue[i] = LowLevelJoystick::Get()->GetJoystickAxis( i );
      stepsize[i] = (newvalue[i] - m_last_axis[i]) / m_HostInfo->BUFSIZE;
    }

  for( int n=0; n<m_HostInfo->BUFSIZE; n++ )
    {
      for( int i=0; i<JP_NUMBER_OF_AXES; i++ )
	SetOutput( i, n, m_last_axis[i] + (n * stepsize[i]) );
      for( int i=0; i<JP_NUMBER_OF_BUTTONS; i++ )
	SetOutput( i+JP_NUMBER_OF_AXES, n, LowLevelJoystick::Get()->GetJoystickButton( i ) );
    }

  for( int i=0; i<JP_NUMBER_OF_AXES; i++ )
    m_last_axis[i] = newvalue[i];

  m_GUI->redraw();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

LowLevelJoystick::LowLevelJoystick() :
  m_InputOk(true)
{ 
  OpenRead(); 
}

//////////////////////////////////////////////////////////////////////

LowLevelJoystick::~LowLevelJoystick()
{
  Close();
}

//////////////////////////////////////////////////////////////////////

void LowLevelJoystick::Read()
{
  struct js_event js;
  if (m_InputOk)
    {
      int ret = read( m_Joyfd, &js, sizeof(struct js_event) );
      if (ret == sizeof(struct js_event))
	{
	switch ((js.type&3))
	  {
	  case JS_EVENT_BUTTON:
	    m_button[js.number] = (float)js.value;
	    break;
	  case JS_EVENT_AXIS:
	    m_axis[js.number] = ((float)js.value) / 32767;
	    break;
	  }
	}
    }
}

//////////////////////////////////////////////////////////////////////

float LowLevelJoystick::GetJoystickAxis( int i )
{
  if( m_invert_axis[i] )
    return( -m_axis[i] );
  else
    return( m_axis[i] );
}

//////////////////////////////////////////////////////////////////////

float LowLevelJoystick::GetJoystickButton( int i )
{
  return( m_button[i] );
}

//////////////////////////////////////////////////////////////////////

void LowLevelJoystick::Close()
{
  cerr<<"Closing joystick"<<endl;
  close(m_Joyfd);
}

//////////////////////////////////////////////////////////////////////

void LowLevelJoystick::OpenRead()
{ 	
  int result,val;
  
  cerr<<"Opening joystick"<<endl;

  m_Joyfd = open("/dev/js0",O_RDONLY);  
  if(m_Joyfd<0) 
  {
    fprintf(stderr,"Can't open joystick driver for reading.\n");
    m_InputOk=false;
    return;
  }

  fcntl(m_Joyfd, F_SETFL, O_NONBLOCK);
  //CHECK_AND_REPORT_ERROR;
}


//////////////////////////////////////////////////////////////////////
