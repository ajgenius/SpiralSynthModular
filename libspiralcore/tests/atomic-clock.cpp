// SPDX-License-Identifier: GPL-2.0-or-later
// The platform clock ticks at the requested rate: 200 ticks at 1 kHz
// take 0.2 s of wall time within a loose tolerance, and the reported
// time advances one period per tick.
#include "AtomicClock.h"
#include <cstdio>
#include <cmath>
#include <sys/time.h>

static double Now()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec + tv.tv_usec / 1e6;
}

int main()
{
	const float frequency = 1000.f;
	const unsigned ticks = 200;
	AtomicClock clock(frequency);

	const double start = Now();
	double reported = 0;
	for (unsigned n = 0; n < ticks; ++n) reported = clock.Tick();
	const double elapsed = Now() - start;

	const double expected = ticks / frequency;
	printf("elapsed %.4f s, reported %.4f s, expected %.4f s\n", elapsed, reported, expected);
	if (std::fabs(reported - expected) > 1e-6) { printf("FAIL: reported time\n"); return 1; }
	if (elapsed < expected * 0.9 || elapsed > expected * 1.5) { printf("FAIL: wall time\n"); return 1; }
	printf("PASS\n");
	return 0;
}
