// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRALCORE_TIMED_AUDIO_BUFFER_H
#define SPIRALCORE_TIMED_AUDIO_BUFFER_H

#include "AudioTiming.h"
#include <vector>

namespace spiralcore
{
// Timestamped SPSC audio. A producer publishes whole blocks. The consumer plays
// samples at requested presentation times, discarding expired audio instead of
// retaining an offset after a missed callback. Configure requires both sides
// stopped. Write/Read allocate nothing, never lock, and have bounded storage.
class TimedAudioBuffer
{
public:
	TimedAudioBuffer();
	bool Configure(unsigned capacityFrames, unsigned channels, double inputRate, double outputRate);
	bool Write(const float *samples, unsigned frames, const AudioStamp &stamp);
	// Returns frames covered by the timeline; uncovered samples are silence.
	unsigned Read(float *samples, unsigned frames, double time, double step, unsigned generation);
	unsigned Capacity() const { return m_Capacity; }
	unsigned Buffered() const;
	unsigned Lookahead() const { return m_Taps / 2; }

private:
	struct Position
	{
		double Time, Step;
		uint64_t Frame;
		unsigned Generation;
	};
	unsigned m_Capacity, m_Mask, m_Channels, m_Taps;
	mutable unsigned m_Read, m_Write;
	unsigned m_Search;
	double m_Cutoff;
	std::vector<float> m_Samples, m_Filter;
	std::vector<Position> m_Positions;
	bool Covered(unsigned index, unsigned read, unsigned write, unsigned generation) const;
	TimedAudioBuffer(const TimedAudioBuffer &);
	TimedAudioBuffer &operator=(const TimedAudioBuffer &);
};
}
#endif
