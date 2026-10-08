// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRALCORE_AUDIO_TIMING_H
#define SPIRALCORE_AUDIO_TIMING_H

#include <stdint.h>

namespace spiralcore
{
// Seconds on the system monotonic clock. Native clocks must be mapped to this
// domain before publishing timestamps; wall time and musical transport are not
// presentation clocks.
double AudioMonotonicTime();

struct AudioCycleTiming
{
	uint64_t Frame;
	double CallbackTime;
	double InputTime;
	double OutputTime;
	double SampleRate;
	double Step;
	unsigned Frames;
	unsigned Epoch;
	bool Valid;
	bool Estimated;

	AudioCycleTiming() : Frame(0), CallbackTime(0), InputTime(0), OutputTime(0),
		SampleRate(0), Step(0), Frames(0), Epoch(0), Valid(false), Estimated(false) {}
};

struct AudioStamp
{
	uint64_t Frame;
	unsigned Generation;
	double Time;
	double Step;

	AudioStamp() : Frame(0), Generation(0), Time(0), Step(0) {}
};

// A bounded single-writer/single-reader latest-value handoff. Each side owns a
// different slot; exchanging the middle slot transfers ownership, never a live
// structure. No reader retries and no callback takes a lock.
class AudioTimingMailbox
{
public:
	AudioTimingMailbox();
	void Publish(const AudioCycleTiming &value);
	bool Read(AudioCycleTiming &value);

private:
	AudioCycleTiming m_Slots[3];
	unsigned m_Back, m_Front;
	unsigned m_Middle;
	AudioTimingMailbox(const AudioTimingMailbox &);
	AudioTimingMailbox &operator=(const AudioTimingMailbox &);
};

// Estimates seconds per native frame. Timestamps drive rate recovery; callback
// arrival jitter must not be mistaken for hardware clock drift. Discontinuous
// observations reset the estimate rather than manufacturing a large rate jump.
class AudioRateEstimator
{
public:
	AudioRateEstimator();
	void Reset();
	double Observe(uint64_t frame, double time, double nominalRate);

private:
	bool m_Ready;
	uint64_t m_Frame;
	double m_Time, m_Step;
};
}
#endif
