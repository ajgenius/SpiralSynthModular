// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRALCORE_PRESENTATION_CLOCK_H
#define SPIRALCORE_PRESENTATION_CLOCK_H
#include "AudioTiming.h"
namespace spiralcore
{
// Engine-owned mapping from monotonically increasing graph frames to physical
// presentation time. Musical locate/pause never rewinds these frame numbers.
class PresentationClock
{
public:
	PresentationClock();
	void Reset();
	bool Prepare(double now, unsigned frames, double rate, const AudioCycleTiming *master,
		double ahead, AudioStamp &stamp);
	void Commit(unsigned frames);
	double Wait() const { return m_Wait; }
	unsigned Generation() const { return m_Generation; }

private:
	bool m_Ready, m_Native;
	unsigned m_Generation, m_MasterEpoch;
	uint64_t m_Frame;
	double m_NextTime, m_TargetFrame, m_MasterRate, m_Rate, m_Ahead, m_Wait, m_Step;
};
}
#endif
