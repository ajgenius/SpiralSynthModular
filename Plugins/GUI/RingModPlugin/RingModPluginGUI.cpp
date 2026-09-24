#include "RingModPlugin.h"
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

#include "RingModPluginGUI.h"
#include <FL/fl_draw.H>
#include <FL/fl_draw.H>

using namespace std;

////////////////////////////////////////////

RingModPluginGUI::RingModPluginGUI (int w, int h, RingModPlugin *o, ChannelHandler *ch, const HostInfo *Info) :
SpiralPluginGUI (w, h, o, ch)
{
	m_Amount = new Fl_Knob (15, 12, 50, 50, "Amount");
    m_Amount->color(Info->GUI_COLOUR);
	m_Amount->type(Fl_Knob::DOTLIN);
    m_Amount->labelsize(10);
    m_Amount->maximum(3);
    m_Amount->step(0.0001);
    m_Amount->value(1);   
	m_Amount->callback((Fl_Callback*)cb_Amount);
	
	end();
}

void RingModPluginGUI::UpdateValues(SpiralPlugin *o)
{
	RingModPlugin* Plugin = (RingModPlugin*)o;
	m_Amount->value(Plugin->GetAmount());
}

inline void RingModPluginGUI::cb_Amount_i(Fl_Knob* o, void* v) 
{ 
	m_GUICH->Set("Amount",(float)o->value()); 
}
void RingModPluginGUI::cb_Amount(Fl_Knob* o, void* v) 
{ ((RingModPluginGUI*)(o->parent()))->cb_Amount_i(o,v); }

const string RingModPluginGUI::GetHelpText(const string &loc){
    return string("")
    + "A ring modulator, multiplies two inputs together.\n"
    + "An extra dial sets the amount of modulation. The\n"
    + "ring modulation effect creates an extra harmonic\n"
    + "in the signal, although this is often out of tune\n"
    + "with the other signals, its useful for creating bell\n"
    + "or struck metallic sounds.\n";
}

#include "SpiralIcon.xpm"

#include <config.h>

extern "C" {
const char *SpiralPlugin_GetHostVersion()
{
	return PACKAGE_VERSION;
}

const char *SpiralPlugin_GetHostABI()
{
	return SSM_HOST_ABI;
}


int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_GUI;
}

int SpiralPlugin_GetID()
{
	return 0x000a;
}

const char **SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}

SpiralGUIType *SpiralPlugin_CreateGUI(SpiralPlugin *plugin)
{
	RingModPlugin *p = (RingModPlugin *)plugin;
	if (!p) return 0;
	return new RingModPluginGUI(p->GetPluginInfo().Width,
	p->GetPluginInfo().Height,
	p,p->GetChannelHandler(),p->GetHostInfo());
}

}
