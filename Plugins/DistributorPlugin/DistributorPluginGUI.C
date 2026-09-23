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

#include "DistributorPluginGUI.h"
#include <FL/fl_draw.H>
#include <FL/fl_draw.H>

using namespace std;

////////////////////////////////////////////

DistributorPluginGUI::DistributorPluginGUI(int w, int h,DistributorPlugin *o,ChannelHandler *ch,const HostInfo *Info) :
SpiralPluginGUI(w,h,o,ch)
{	
	m_Chans = new Fl_Counter (15, 20, 50, 15, "Channels");
	m_Chans->labelsize (8);
	m_Chans->textsize (8);
	m_Chans->type (FL_SIMPLE_COUNTER);
	m_Chans->box (FL_PLASTIC_UP_BOX);
	m_Chans->color (Info->GUI_COLOUR);
	m_Chans->selection_color (Info->GUI_COLOUR);
	m_Chans->step (1);
	m_Chans->value (2);
	m_Chans->callback ((Fl_Callback*) cb_Chans, this);
	add (m_Chans);

	end();
}


void DistributorPluginGUI::Update()
{
}

void DistributorPluginGUI::UpdateValues(SpiralPlugin *o)
{
	DistributorPlugin* Plugin = (DistributorPlugin*)o;
	m_Chans->value (Plugin->GetChannelCount());
}
	
inline void DistributorPluginGUI::cb_Chans_i (Fl_Counter* o) 
{
	if (o->value() < 2) 
	{
		o->value(2);
	}	
	else {
		m_GUICH->Set ("ChannelCount", int (o->value ()));
		m_GUICH->SetCommand (DistributorPlugin::SETCHANNELCOUNT);
		m_GUICH->Wait ();
		Resize (w(), h());
	}
}

const string DistributorPluginGUI::GetHelpText(const string &loc)
{
	return string("")
	+ "The Distributor plugin is for polyphony. NEED MORE INFO HERE.\n";
}
