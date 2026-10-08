// SPDX-License-Identifier: GPL-2.0-or-later
// Mach absolute-time AtomicClock for macOS. Each Tick waits for an
// absolute deadline computed from the start, so periods do not
// accumulate sleep error.
#include "AtomicClock.h"
#include <mach/mach_time.h>
#include <stdint.h>

struct AtomicClock::Platform
{
	uint64_t m_Start;
	uint64_t m_Period;
	uint64_t m_Ticks;
};

AtomicClock::AtomicClock(float frequency) :
m_Platform(new Platform),
m_Time(0),
m_Frequency(frequency)
{
	mach_timebase_info_data_t timebase;
	mach_timebase_info(&timebase);
	// Period in mach units: nanoseconds * denom / numer.
	const double nanoseconds = 1e9 / frequency;
	m_Platform->m_Period = (uint64_t)(nanoseconds * timebase.denom / timebase.numer);
	m_Platform->m_Ticks = 0;
	m_Platform->m_Start = mach_absolute_time();
}

AtomicClock::~AtomicClock()
{
	delete m_Platform;
}

double AtomicClock::Tick()
{
	Platform &p = *m_Platform;
	++p.m_Ticks;
	mach_wait_until(p.m_Start + p.m_Ticks * p.m_Period);
	m_Time = p.m_Ticks / (double)m_Frequency;
	return m_Time;
}
