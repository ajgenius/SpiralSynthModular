// SPDX-License-Identifier: GPL-2.0-or-later
#include "AudioTiming.h"
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

using namespace Spiral::Core;

static bool CheckReadings()
{
	double previous = AudioMonotonicTime();
	if (!(previous > 0)) return false;

	for (unsigned n = 0; n < 100000; ++n)
	{
		const double now = AudioMonotonicTime();
		if (!(now >= previous)) return false;

		previous = now;
	}

	return true;
}

#ifdef _WIN32
static DWORD WINAPI ReadClock(void *)
{
	return CheckReadings() ? 0 : 1;
}
#endif

int main()
{
#ifdef _WIN32
	// Exercise simultaneous first use, including 32-bit counter reads.
	HANDLE readers[4];
	for (unsigned n = 0; n < 4; ++n)
	{
		readers[n] = CreateThread(0, 0, ReadClock, 0, 0, 0);
		if (!readers[n]) return 1;
	}

	bool passed = true;
	for (unsigned n = 0; n < 4; ++n)
	{
		DWORD result = 1;
		if (WaitForSingleObject(readers[n], INFINITE) != WAIT_OBJECT_0
			|| !GetExitCodeThread(readers[n], &result) || result != 0)
			passed = false;

		CloseHandle(readers[n]);
	}

	if (!passed) return 1;

	const DWORD wallStart = GetTickCount();
#else
	if (!CheckReadings()) return 1;

	timeval wallStart;
	gettimeofday(&wallStart, 0);
#endif
	const double start = AudioMonotonicTime();
#ifdef _WIN32
	Sleep(120);
	const double wallElapsed = DWORD(GetTickCount() - wallStart) / 1000.0;
#else
	usleep(120000);
	timeval wallEnd;
	gettimeofday(&wallEnd, 0);
	const double wallElapsed = wallEnd.tv_sec - wallStart.tv_sec
		+ (wallEnd.tv_usec - wallStart.tv_usec) / 1e6;
#endif
	const double elapsed = AudioMonotonicTime() - start;
	printf("monotonic %.6f s, independent clock %.6f s\n", elapsed, wallElapsed);
	if (!(elapsed >= 0.1) || elapsed - wallElapsed > 0.05 || wallElapsed - elapsed > 0.05)
		return 1;

	printf("PASS monotonic clock\n");
	return 0;
}
