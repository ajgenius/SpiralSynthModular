// SPDX-License-Identifier: GPL-2.0-or-later
#include "AtomicClock.h"
#include <windows.h>
#include <cmath>
#include <limits>
#include <stdexcept>

struct AtomicClock::Platform
{
	HANDLE Timer;
	LARGE_INTEGER Start;
	LARGE_INTEGER Rate;
	unsigned long long Ticks;
};

AtomicClock::AtomicClock(float frequency) :
m_Platform(0),
m_Time(0),
m_Frequency(frequency)
{
	// A relative wait must fit the timer's signed count of 100 ns units.
	if (!(frequency >= 1e7 / double(std::numeric_limits<LONGLONG>::max()))
		|| frequency > std::numeric_limits<float>::max())
		throw std::invalid_argument("AtomicClock requires a finite positive frequency");

	Platform *p = new Platform;
	p->Timer = 0;

	// Resolve the newer entry point at runtime so older Windows can still load
	// the core. High-resolution timers are available from Windows 10 1803.
	typedef HANDLE (WINAPI *CreateTimer)(LPSECURITY_ATTRIBUTES, LPCWSTR, DWORD, DWORD);
	CreateTimer createTimer = reinterpret_cast<CreateTimer>(reinterpret_cast<void (*)()>(
		GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "CreateWaitableTimerExW")));
	if (createTimer)
		p->Timer = createTimer(0, 0, 0x00000002, TIMER_MODIFY_STATE | SYNCHRONIZE);

	if (!p->Timer)
		p->Timer = CreateWaitableTimerW(0, FALSE, 0);

	if (!p->Timer)
	{
		delete p;
		throw std::runtime_error("AtomicClock could not create a waitable timer");
	}

	QueryPerformanceFrequency(&p->Rate);
	QueryPerformanceCounter(&p->Start);
	p->Ticks = 0;
	m_Platform = p;
}

AtomicClock::~AtomicClock()
{
	CloseHandle(m_Platform->Timer);
	delete m_Platform;
}

double AtomicClock::Tick()
{
	Platform &p = *m_Platform;
	const double deadline = ++p.Ticks / double(m_Frequency);

	// Recompute the remaining wait against a deadline from the original start.
	// Timer overshoot cannot accumulate into drift; late ticks catch up just as
	// they do with MachAtomicClock. Never use an adjustable wall-clock deadline.
	for (;;)
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		const double remaining = deadline - double(now.QuadPart - p.Start.QuadPart) / p.Rate.QuadPart;
		if (remaining <= 0) break;

		LARGE_INTEGER due;
		due.QuadPart = -LONGLONG(std::ceil(remaining * 1e7));
		if (!SetWaitableTimer(p.Timer, &due, 0, 0, 0, FALSE)
			|| WaitForSingleObject(p.Timer, INFINITE) != WAIT_OBJECT_0)
			throw std::runtime_error("AtomicClock wait failed");
	}

	m_Time = deadline;
	return m_Time;
}
