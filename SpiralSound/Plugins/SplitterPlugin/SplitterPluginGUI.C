/*  SpiralPlugin
 *  Copyleft (C) 2000 David Griffiths <dave@pawfal.org>
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

#include "SplitterPluginGUI.h"
#include <FL/fl_draw.h>
#include <FL/fl_draw.H>

using namespace std;

////////////////////////////////////////////

SplitterPluginGUI::SplitterPluginGUI(int w, int h,SplitterPlugin *o,ChannelHandler *ch,const HostInfo *Info) :
SpiralPluginGUI(w,h,o,ch)
{	
	m_Channels = new Fl_Counter (15, 20, 50, 15, "Channels");
	m_Channels->labelsize (8);
	m_Channels->textsize (8);
	m_Channels->type (FL_SIMPLE_COUNTER);
	m_Channels->box (FL_PLASTIC_UP_BOX);
	m_Channels->color (Info->GUI_COLOUR);
	m_Channels->selection_color (Info->GUI_COLOUR);
	m_Channels->step (1);
	m_Channels->value (4);
	m_Channels->callback ((Fl_Callback*) cb_Channels, this);
	add (m_Channels);

	end();
}


void SplitterPluginGUI::Update()
{
}

void SplitterPluginGUI::UpdateValues(SpiralPlugin *o)
{
	SplitterPlugin* Plugin = (SplitterPlugin*)o;
	m_Channels->value (Plugin->GetChannelCount());
}
	
inline void SplitterPluginGUI::cb_Channels_i (Fl_Counter* o) 
{
	if (o->value() < 2) 
	{
		o->value(2);
	}	
	else {
		m_GUICH->Set ("ChannelCount", int (o->value ()));
		m_GUICH->SetCommand (SplitterPlugin::SETCHANNELCOUNT);
		m_GUICH->Wait ();
		Resize (w(), h());
	}
}

const string SplitterPluginGUI::GetHelpText(const string &loc)
{
	return string("")
	+ "The simplest plugin - the splitter simply takes the input,\n"
	+ "and duplicates it into it's outputs. Simple, but difficult\n"
	+ "to do without.\n";
}
