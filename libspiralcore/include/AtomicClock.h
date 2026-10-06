// Copyright (C) 2004 David Griffiths <dave@pawfal.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifndef ATOMIC_CLOCK
#define ATOMIC_CLOCK

// A steady periodic clock for pacing the engine when no audio backend
// does. Tick() blocks until the next period and returns the time since
// the clock started, in seconds. Configure selects the one platform
// implementation: Mach in core, or AlsaAtomicClock in the ALSA module.
// The active audio scheduler uses AudioTimeline and does not depend on it.
class AtomicClock
{
public:
	AtomicClock(float frequency);
	~AtomicClock();
	double Tick();
	float Frequency() const { return m_Frequency; }
	
private:
	struct Platform;
	Platform *m_Platform;
	
	double m_Time;
	float m_Frequency;
};

#endif
