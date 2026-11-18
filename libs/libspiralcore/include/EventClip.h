// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRALCORE_EVENT_CLIP_H
#define SPIRALCORE_EVENT_CLIP_H

#include "Event.h"
#include <stdint.h>
#include <string>
#include <vector>

namespace Spiral
{
namespace Core
{
	struct NoteSpan
	{
		Event Note;
		uint64_t Start, End;
		unsigned OnEvent, OffEvent;
		NoteSpan(): Start(0), End(0), OnEvent(~0u), OffEvent(~0u) {}
		bool Closed() const { return OffEvent != ~0u; }
	};

	class EventClip
	{
	public:
		enum
		{
			MaxEvents = 65536
		};

		EventClip();
		~EventClip();

		void Clear(unsigned rate);
		bool Append(uint64_t frame, const Event &event);
		const Event &At(unsigned index) const { return m_Events[index]; }
		uint64_t FrameAt(unsigned index) const { return m_Frames[index]; }
		unsigned Count() const { return m_Count; }
		unsigned Rate() const { return m_Rate; }
		uint64_t Length() const { return m_Length; }
		void Extend(uint64_t frames);
		void SetLength(uint64_t frames);
		void Notes(std::vector<NoteSpan> &notes) const;
		bool EditNote(unsigned on, uint64_t start, uint64_t end, unsigned key, float velocity, bool erase = false);
		bool AddNote(uint64_t start, uint64_t end, unsigned channel, unsigned key, float velocity);

		// For recording over part of a clip. CopyBefore makes this the part
		// of a clip before a frame, a note sounding across the frame ended at
		// it, so a take can be appended from there. AppendFrom then puts
		// back what of that clip lies at or after the frame the take
		// stopped at: notes the take left sounding are ended there, and a
		// note of the clip sounding across it starts again there.
		bool CopyBefore(const EventClip &source, uint64_t frame);
		bool AppendFrom(const EventClip &source, uint64_t frame);
		std::string Save() const;
		bool Load(const std::string &text);
		void Swap(EventClip &other);

	private:
		EventClip(const EventClip &);
		EventClip &operator=(const EventClip &);
		bool Rewrite(unsigned on, unsigned off, const Event *start, uint64_t startFrame, const Event *end,
				uint64_t endFrame);

		Event *m_Events;
		uint64_t *m_Frames;
		unsigned m_Count, m_Rate;
		uint64_t m_Length;
	};
}
}

#endif
