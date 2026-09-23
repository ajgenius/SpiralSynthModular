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

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Box.H>
#include "Fl_Knob.H"
#include "JoystickPlugin.h"
#include "SpiralPluginGUI.h"
#include <stdio.h>

#ifndef JOY_GUI_H
#define JOY_GUI_H

class JoystickPluginGUI : public SpiralPluginGUI
{
public:
	JoystickPluginGUI(int w, int h, JoystickPlugin *o,const HostInfo *Info);

	virtual void draw();
	virtual void UpdateValues();
	virtual SpiralPlugin* GetPlugin() { return m_Plugin; }

	JoystickPlugin *m_Plugin;

private:
	Fl_Button *Invertx;
	Fl_Button *Inverty;
	Fl_Button *Invertz;
	Fl_Box *valuex;
	Fl_Box *valuey;
	Fl_Box *valuez;

	//// Callbacks ////
	inline void cb_Invert_ix(Fl_Button* o, void* v);
	static void cb_Invertx(Fl_Button* o, void* v);
	inline void cb_Invert_iy(Fl_Button* o, void* v);
	static void cb_Inverty(Fl_Button* o, void* v);
	inline void cb_Invert_iz(Fl_Button* o, void* v);
	static void cb_Invertz(Fl_Button* o, void* v);
};

#endif
