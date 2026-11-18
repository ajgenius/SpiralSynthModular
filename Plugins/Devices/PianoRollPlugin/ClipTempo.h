// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SSM_CLIP_TEMPO_H
#define SSM_CLIP_TEMPO_H
#include <stdint.h>

class ClipTempo
{
public:
	ClipTempo(): fraction(0) {}
	void Reset() { fraction = 0; }

	static bool Valid(double bpm) { return bpm >= 5 && bpm <= 900; }

	uint64_t Advance(double step)
	{
		const double total = fraction + step;
		const uint64_t whole = static_cast<uint64_t>(total);
		fraction = total - whole;
		return whole;
	}

private:
	double fraction;
};

#endif
