// SPDX-License-Identifier: GPL-2.0-or-later
#include "AudioTimeline.h"
#include <cassert>
#include <cstdio>
using namespace spiralcore;

int main()
{
	AudioTimeline first, second;
	assert(first.Prepare(256, 48000, 10));
	first.Begin();
	const AudioStamp initial = first.PlaybackStamp();
	first.Commit(256);
	assert(first.Frame() == 256 && second.Frame() == 0);
	assert(!first.Prepare(256, 48000, 10));
	assert(first.SleepMicroseconds() > 0 && first.SleepMicroseconds() <= 1000);

	first.Stop();
	assert(first.Prepare(256, 48000, 10 + 256 / 48000.0));
	first.Begin();
	first.Commit(256);
	assert(first.Frame() == 256 && !first.Rolling());
	assert(first.PlaybackStamp().Frame == initial.Frame + 256);

	first.Locate(9000);
	assert(first.Prepare(256, 48000, 11));
	assert(first.PlaybackStamp().Generation != initial.Generation);
	assert(first.PlaybackStamp().Frame > initial.Frame);
	first.Start();
	first.Commit(256);
	assert(first.Frame() == 9256);
	assert(second.Prepare(128, 44100, 11));
	assert(second.PlaybackStamp().Frame == 0 && second.Frame() == 0);
	assert(!second.Prepare(0, 44100, 11));
	puts("Independent engine timelines, nominal pacing, locate and pause PASS");
}
