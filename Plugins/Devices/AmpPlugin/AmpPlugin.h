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

#ifndef AMPPLUGIN
#define AMPPLUGIN

#include "SpiralPlugin.h"

class AmpPlugin : public SpiralPlugin
{
public:
 	AmpPlugin();
	virtual ~AmpPlugin();

	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void Describe(Spiral::Core::Description &d);
	virtual void Apply(Spiral::Core::Description::Reader &r);

	// has to be defined in the plugin

	float GetGain() { return m_Gain; }
	float GetDC() { return m_DC;  }

	void Randomise();

private:
	float m_Gain, m_DC;
	friend Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, AmpPlugin &o);
	friend Spiral::Core::Description &Describe(Spiral::Core::Description &d, AmpPlugin &o);
};
Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, AmpPlugin &o);
Spiral::Core::Description &Describe(Spiral::Core::Description &d, AmpPlugin &o);

#endif
