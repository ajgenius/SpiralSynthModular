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

#include "JoystickPluginGUI.h"
#include <FL/fl_draw.H>

using namespace std;

JoystickPluginGUI::JoystickPluginGUI(int w, int h,JoystickPlugin *o,const HostInfo *Info) :
  SpiralPluginGUI(w,h,o)
{	
  m_Plugin=o;

  Invertx = new Fl_Button(0, 20, 50, 20, "Invert x");
  Invertx->type(1);
  Invertx->down_box(FL_DOWN_BOX);
  Invertx->labelsize(10);
  Invertx->callback((Fl_Callback*)cb_Invertx);

  Inverty = new Fl_Button(50, 20, 50, 20, "Invert y");
  Inverty->type(1);
  Inverty->down_box(FL_DOWN_BOX);
  Inverty->labelsize(10);
  Inverty->callback((Fl_Callback*)cb_Inverty);

  Invertz = new Fl_Button(100, 20, 50, 20, "Invert z");
  Invertz->type(1);
  Invertz->down_box(FL_DOWN_BOX);
  Invertz->labelsize(10);
  Invertz->callback((Fl_Callback*)cb_Invertz);

  valuex = new Fl_Box( 0, 40, 50, 20 );
  valuey = new Fl_Box( 50, 40, 50, 20 );
  valuez = new Fl_Box( 100, 40, 50, 20 );
  
  end();
}

char xbuffer[20];
char ybuffer[20];
char zbuffer[20];

void JoystickPluginGUI::UpdateValues()
{
  snprintf( xbuffer, 20, "%.3f", LowLevelJoystick::Get()->GetJoystickAxis( 0 ) );
  valuex->label( xbuffer );
  snprintf( ybuffer, 20, "%.3f", LowLevelJoystick::Get()->GetJoystickAxis( 1 ) );
  valuey->label( ybuffer );
  snprintf( zbuffer, 20, "%.3f", LowLevelJoystick::Get()->GetJoystickAxis( 2 ) );
  valuez->label( zbuffer );
  Fl::check();
}

void JoystickPluginGUI::draw()
{
  SpiralGUIType::draw();
  UpdateValues();
}

inline void JoystickPluginGUI::cb_Invert_ix(Fl_Button* o, void* v)
{ LowLevelJoystick::Get()->m_invert_axis[0] = o->value(); }
void JoystickPluginGUI::cb_Invertx(Fl_Button* o, void* v)
{ ((JoystickPluginGUI*)(o->parent()))->cb_Invert_ix(o,v); }

inline void JoystickPluginGUI::cb_Invert_iy(Fl_Button* o, void* v)
{ LowLevelJoystick::Get()->m_invert_axis[1] = o->value(); }
void JoystickPluginGUI::cb_Inverty(Fl_Button* o, void* v)
{ ((JoystickPluginGUI*)(o->parent()))->cb_Invert_iy(o,v); }

inline void JoystickPluginGUI::cb_Invert_iz(Fl_Button* o, void* v)
{ LowLevelJoystick::Get()->m_invert_axis[2] = o->value(); }
void JoystickPluginGUI::cb_Invertz(Fl_Button* o, void* v)
{ ((JoystickPluginGUI*)(o->parent()))->cb_Invert_iz(o,v); }
