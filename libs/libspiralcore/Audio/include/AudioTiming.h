// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRALCORE_AUDIO_TIMING_H
#define SPIRALCORE_AUDIO_TIMING_H

#include <stdint.h>

namespace Spiral
{
namespace Core
{
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
	// This epoch follows a recovered write on the same playback device.
	bool PlaybackRestart;

	AudioCycleTiming() : Frame(0), CallbackTime(0), InputTime(0), OutputTime(0),
		SampleRate(0), Step(0), Frames(0), Epoch(0), Valid(false), Estimated(false), PlaybackRestart(false) {}
};

struct AudioStamp
{
	uint64_t Frame;
	unsigned Generation;
	double Time;
	double Step;

	AudioStamp() : Frame(0), Generation(0), Time(0), Step(0) {}
};

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
}
#endif
