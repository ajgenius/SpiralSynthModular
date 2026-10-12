// SPDX-License-Identifier: GPL-2.0-or-later
// The platform clock ticks at the requested rate: 100 ticks at 250 Hz
// take 0.4 s of wall time within a loose tolerance, and the reported
// time tracks the wall clock. 250 Hz stays inside what a virtual
// machine's system timer can deliver.
#include "AtomicClock.h"
#include <cstdio>
#include <cmath>
#ifdef _WIN32
#include <windows.h>
#include <limits>
#include <stdexcept>
#else
#include <sys/time.h>
#endif
#ifdef __linux__
#include <unistd.h>
#endif

static double Now()
{
#ifdef _WIN32
	LARGE_INTEGER now, rate;
	QueryPerformanceCounter(&now);
	QueryPerformanceFrequency(&rate);
	return double(now.QuadPart) / rate.QuadPart;
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec + tv.tv_usec / 1e6;
#endif
}

int main()
{
#ifdef __linux__
	// The optional ALSA timer requires a usable kernel timer device.
	if (access("/dev/snd/timer", R_OK | W_OK) != 0) return 77;
#endif
	const float frequency = 250.f;
	const unsigned ticks = 100;
	AtomicClock clock(frequency);

	const double start = Now();
	double reported = 0;
	for (unsigned n = 0; n < ticks; ++n) reported = clock.Tick();
	const double elapsed = Now() - start;

	const double expected = ticks / frequency;
	printf("elapsed %.4f s, reported %.4f s, expected %.4f s\n", elapsed, reported, expected);
	if (std::fabs(reported - elapsed) > expected * 0.1) { printf("FAIL: reported time\n"); return 1; }
	if (elapsed < expected * 0.9 || elapsed > expected * 1.5) { printf("FAIL: wall time\n"); return 1; }
#ifdef _WIN32
	// A late caller must retain the original schedule, not restart its period.
	Sleep(40);
	for (unsigned n = 0; n < ticks; ++n) reported = clock.Tick();

	const double caughtUp = Now() - start;
	printf("after late caller %.4f s, reported %.4f s\n", caughtUp, reported);
	if (std::fabs(reported - 2 * expected) > 0.0001
		|| std::fabs(caughtUp - 2 * expected) > 0.035)
		return 1;

	// Repeated construction must release every native timer handle.
	DWORD handlesBefore, handlesAfter;
	if (!GetProcessHandleCount(GetCurrentProcess(), &handlesBefore)) return 1;

	for (unsigned n = 0; n < 100; ++n)
	{
		AtomicClock temporary(333.3f);
		temporary.Tick();
	}

	if (!GetProcessHandleCount(GetCurrentProcess(), &handlesAfter)
		|| handlesAfter > handlesBefore)
		return 1;

	const float invalid[] = {0, -1, std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::quiet_NaN()};
	for (unsigned n = 0; n < sizeof(invalid) / sizeof(invalid[0]); ++n)
	{
		try
		{
			AtomicClock bad(invalid[n]);
			return 1;
		}
		catch (const std::invalid_argument &)
		{
		}
	}
#endif
	printf("PASS\n");
	return 0;
}
