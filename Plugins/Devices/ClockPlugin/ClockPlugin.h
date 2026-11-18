// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CLOCK_PLUGIN
#define CLOCK_PLUGIN

#include "Device.h"

#include "MusicalClock.h"

class ClockPlugin : public Spiral::Core::Device
{
public:
	ClockPlugin();
	virtual void Execute();
	virtual void Reset();
	virtual void Apply(Spiral::Core::Description::Reader &reader);

private:
	Spiral::Core::FloatProperty m_Tempo;
	Spiral::Core::SignedProperty m_Beats, m_BeatUnit, m_Steps, m_Bars, m_TempoNote, m_StepNote;
	MusicalClock m_Grid;
};

#endif
