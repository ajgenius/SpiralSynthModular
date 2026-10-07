// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRALCORE_AUDIO_TIMELINE_H
#define SPIRALCORE_AUDIO_TIMELINE_H
#include "AudioStream.h"
#include "PresentationClock.h"
#include <vector>

namespace spiralcore
{
// One timeline per engine. The host excludes graph/control edits while calling
// these methods; native callbacks own only their AudioStream queue ends.
// Register/unregister before starting or destroying streams. No host/GUI types
// cross this boundary, so both SSM engines use the same scheduling policy.
class AudioTimeline
{
public:
	AudioTimeline();
	void Register(AudioStream *stream);
	void Unregister(AudioStream *stream);
	void Reset();
	bool Prepare(unsigned frames, double rate, double now, AudioStream *preferred = NULL);
	unsigned SleepMicroseconds() const;
	void Begin();
	void Commit(unsigned frames);
	const AudioStamp &PlaybackStamp() const { return m_PlaybackStamp; }
	const AudioStamp &CaptureStamp() const { return m_CaptureStamp; }
	unsigned long Frame() const { return m_Frame; }
	bool Rolling() const { return m_Rolling; }
	void Start();
	void Stop();
	void Locate(unsigned long frame);

private:
	AudioClient *MasterClient() const;
	std::vector<AudioStream *> m_Streams;
	AudioStream *m_Master;
	PresentationClock m_Presentation;
	AudioStamp m_PlaybackStamp, m_CaptureStamp;
	unsigned long m_Frame;
	bool m_Rolling;
	AudioTimeline(const AudioTimeline &);
	AudioTimeline &operator=(const AudioTimeline &);
};
}
#endif
