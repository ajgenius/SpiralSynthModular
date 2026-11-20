// SPDX-License-Identifier: GPL-2.0-or-later
#include "EventClip.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <set>
#include <iomanip>
#include <locale>
#include <sstream>

namespace Spiral
{
namespace Core
{
	static const uint64_t MaxSeconds = 86400;

	EventClip::EventClip():
		m_Events(new Event[MaxEvents]),
		m_Count(0),
		m_Rate(48000),
		m_Length(0)
	{
	}

	EventClip::~EventClip()
	{
		delete[] m_Events;
	}

	void EventClip::Clear(unsigned rate)
	{
		m_Count = 0;
		m_Length = 0;
		m_Rate = std::max(1u, std::min(768000u, rate));
	}

	bool EventClip::Append(uint64_t frame, const Event &event)
	{
		if (!event.Valid() || m_Count == MaxEvents || frame >= uint64_t(m_Rate) * MaxSeconds
				|| (m_Count && m_Events[m_Count - 1].TimeStamp.Frame > frame))
			return false;

		// An event's frame is kept in its own time stamp, as the private
		// tree's clip keeps it.
		m_Events[m_Count] = event;
		m_Events[m_Count++].TimeStamp.Frame = frame;
		m_Length = std::max(m_Length, frame + 1);

		return true;
	}

	void EventClip::Extend(uint64_t frames)
	{
		m_Length = std::max(m_Length, std::min(frames, uint64_t(m_Rate) * MaxSeconds));
	}

	void EventClip::SetLength(uint64_t frames)
	{
		const uint64_t last = m_Count ? m_Events[m_Count - 1].TimeStamp.Frame + 1 : 0;
		m_Length = std::max(last, std::min(frames, uint64_t(m_Rate) * MaxSeconds));
	}

	void EventClip::Swap(EventClip &other)
	{
		std::swap(m_Events, other.m_Events);
		std::swap(m_Count, other.m_Count);
		std::swap(m_Rate, other.m_Rate);
		std::swap(m_Length, other.m_Length);
	}

	void EventClip::Notes(std::vector<NoteSpan> &notes) const
	{
		typedef std::pair<int, int> Identity;
		// Sort sounding notes by identity and then note-on order. Reused IDs
		// release the oldest sounding note first.
		typedef std::pair<Identity, unsigned> SoundingNote;
		std::set<SoundingNote> held;
		notes.clear();

		for (unsigned i = 0; i < m_Count; ++i)
		{
			const Event &event = m_Events[i];

			if (event.Type == Event::Reset)
			{
				held.clear();

				continue;
			}

			if (event.Type != Event::NoteOn && event.Type != Event::NoteOff)
				continue;

			const Identity identity(event.Channel, event.ID);

			if (event.Type == Event::NoteOn)
			{
				NoteSpan note;
				note.Note = event;
				note.Start = m_Events[i].TimeStamp.Frame;
				note.OnEvent = i;
				note.End = m_Length;
				held.insert(SoundingNote(identity, (unsigned)notes.size()));
				notes.push_back(note);
			}
			else
			{
				const std::set<SoundingNote>::iterator oldest = held.lower_bound(SoundingNote(identity, 0));
				if (oldest == held.end() || oldest->first != identity)
					continue;
				NoteSpan &note = notes[oldest->second];
				note.End = m_Events[i].TimeStamp.Frame;
				note.OffEvent = i;
				held.erase(oldest);
			}
		}
	}

	bool EventClip::CopyBefore(const EventClip &source, uint64_t frame)
	{
		std::vector<NoteSpan> notes;

		source.Notes(notes);
		Clear(source.m_Rate);

		for (unsigned i = 0; i < source.m_Count && source.m_Events[i].TimeStamp.Frame < frame; ++i)
			if (!Append(source.m_Events[i].TimeStamp.Frame, source.m_Events[i]))
				return false;

		const unsigned kept = m_Count;

		for (size_t i = 0; i < notes.size(); ++i)
		{
			const NoteSpan &note = notes[i];

			// Begun before the frame and not over before it.
			if (note.OnEvent >= kept || (note.Closed() && note.OffEvent < kept))
				continue;

			Event off = note.Note;

			off.Type = Event::NoteOff;

			if (!Append(frame, off))
				return false;
		}

		Extend(std::min(frame, source.m_Length));

		return true;
	}

	bool EventClip::AppendFrom(const EventClip &source, uint64_t frame)
	{
		if (source.m_Rate != m_Rate)
			return false;

		std::vector<NoteSpan> notes;

		Notes(notes);

		for (size_t i = 0; i < notes.size(); ++i)
			if (!notes[i].Closed())
			{
				Event off = notes[i].Note;

				off.Type = Event::NoteOff;

				if (!Append(frame, off))
					return false;
			}

		unsigned first = 0;

		while (first < source.m_Count && source.m_Events[first].TimeStamp.Frame < frame)
			++first;

		// The release of a note that ends exactly where the take does has no
		// note left to release.
		std::vector<bool> spent(source.m_Count, false);

		source.Notes(notes);

		for (size_t i = 0; i < notes.size(); ++i)
		{
			const NoteSpan &note = notes[i];

			if (note.OnEvent >= first || (note.Closed() && note.OffEvent < first))
				continue;

			if (note.Closed() && note.End == frame)
			{
				spent[note.OffEvent] = true;

				continue;
			}

			if (!Append(frame, note.Note))
				return false;
		}

		for (unsigned i = first; i < source.m_Count; ++i)
			if (!spent[i] && !Append(source.m_Events[i].TimeStamp.Frame, source.m_Events[i]))
				return false;

		Extend(source.m_Length);

		return true;
	}

	namespace
	{
		struct OrderedEvent
		{
			Event Value;
			uint64_t Frame;
			unsigned Original;

			OrderedEvent(const Event &value, uint64_t frame, unsigned original):
				Value(value),
				Frame(frame),
				Original(original)
			{
			}

			bool operator<(const OrderedEvent &other) const
			{
				return Frame < other.Frame;
			}
		};
	}

	bool EventClip::Rewrite(unsigned on, unsigned off, const Event *start, uint64_t startFrame, const Event *end,
			uint64_t endFrame)
	{
		std::vector<NoteSpan> before;
		Notes(before);
		std::vector<OrderedEvent> ordered;
		ordered.reserve(m_Count + 2);

		for (unsigned i = 0; i < m_Count; ++i)
		{
			if (i != on && i != off)
				ordered.push_back(OrderedEvent(m_Events[i], m_Events[i].TimeStamp.Frame, i));
			else if (start)
				ordered.push_back(i == on ? OrderedEvent(*start, startFrame, i) : OrderedEvent(*end, endFrame, i));
		}

		const bool adding = on == ~0u && start;

		if (adding)
		{
			on = m_Count;
			off = m_Count + 1;
			ordered.push_back(OrderedEvent(*start, startFrame, on));
			ordered.push_back(OrderedEvent(*end, endFrame, off));
		}

		std::stable_sort(ordered.begin(), ordered.end());
		EventClip next;
		next.Clear(m_Rate);
		std::vector<unsigned> position(m_Count + 2, ~0u);

		for (unsigned i = 0; i < ordered.size(); ++i)
		{
			if (!next.Append(ordered[i].Frame, ordered[i].Value))
				return false;

			position[ordered[i].Original] = i;
		}

		next.Extend(m_Length);
		std::vector<NoteSpan> after;
		next.Notes(after);
		std::vector<unsigned> partners(next.Count(), ~0u);

		for (unsigned i = 0; i < after.size(); ++i)
			partners[after[i].OnEvent] = after[i].OffEvent;

		for (unsigned i = 0; i < before.size(); ++i)
		{
			const NoteSpan &note = before[i];

			if (!start && note.OnEvent == on)
				continue;

			const unsigned expected = note.Closed() ? position[note.OffEvent] : ~0u;

			if (partners[position[note.OnEvent]] != expected)
				return false;
		}

		if (adding && partners[position[on]] != position[off])
			return false;

		Swap(next);

		return true;
	}

	static float Frequency(unsigned key)
	{
		return (float)(440.0 * std::pow(2.0, (int(key) - 69) / 12.0));
	}

	bool EventClip::EditNote(unsigned on, uint64_t start, uint64_t end, unsigned key, float velocity, bool erase)
	{
		std::vector<NoteSpan> notes;
		Notes(notes);

		for (unsigned i = 0; i < notes.size(); ++i)
		{
			const NoteSpan &note = notes[i];

			if (note.OnEvent != on)
				continue;

			if (erase)
				return Rewrite(on, note.OffEvent, NULL, 0, NULL, 0);

			if (!note.Closed() || end < start || key > 127 || velocity <= 0 || velocity > 1)
				return false;

			Event first = m_Events[on];
			Event last = m_Events[note.OffEvent];

			if (key != unsigned(first.NoteNum))
				first.Frequency = last.Frequency = Frequency(key);

			first.NoteNum = last.NoteNum = key;
			first.Volume = velocity;

			return Rewrite(on, note.OffEvent, &first, start, &last, end);
		}

		return false;
	}

	bool EventClip::AddNote(uint64_t start, uint64_t end, unsigned channel, unsigned key, float velocity)
	{
		if (end < start || channel > 15 || key > 127 || velocity <= 0 || velocity > 1)
			return false;

		int identity = 0;

		for (unsigned i = 0; i < m_Count; ++i)
			identity = std::max(identity, m_Events[i].ID);

		if (identity == INT_MAX)
			return false;

		Event first;
		first.ID = identity + 1;
		first.Channel = channel;
		first.NoteNum = key;
		first.Frequency = Frequency(key);
		first.Volume = velocity;
		Event last = first;
		last.Type = Event::NoteOff;

		return Rewrite(~0u, ~0u, &first, start, &last, end);
	}

	std::string EventClip::Save() const
	{
		std::ostringstream out;
		out.imbue(std::locale::classic());
		out << "EVENTS1 " << m_Rate << ' ' << m_Length << '\n' << std::setprecision(9);

		for (unsigned i = 0; i < m_Count; ++i)
		{
			const Event &e = m_Events[i];
			out << m_Events[i].TimeStamp.Frame << ' ' << int(e.Type) << ' ' << e.ID << ' ' << e.Channel << ' ' << e.NoteNum << ' '
				<< e.Frequency << ' ' << e.Volume << ' ' << e.Parameter << ' ' << e.Value << ' ' << e.SlideFrequency
				<< ' ' << e.Pan << ' ' << e.Position << ' ' << int(e.Message) << '\n';
		}

		return out.str();
	}

	bool EventClip::Load(const std::string &text)
	{
		if (text.size() > 16u * 1024 * 1024)
			return false;

		std::istringstream input(text);
		input.imbue(std::locale::classic());
		std::string magic;
		unsigned rate;
		uint64_t duration;

		if (!(input >> magic >> rate >> duration) || magic != "EVENTS1" || !rate || rate > 768000
				|| duration > uint64_t(rate) * MaxSeconds)
			return false;

		EventClip next;
		next.Clear(rate);

		while (input >> std::ws && !input.eof())
		{
			Event event;
			uint64_t frame;
			int kind, message;

			if (!(input >> frame >> kind >> event.ID >> event.Channel >> event.NoteNum >> event.Frequency >> event.Volume
					>> event.Parameter >> event.Value >> event.SlideFrequency >> event.Pan >> event.Position >> message)
					|| kind < Event::NoteOn || kind > Event::Reset || message < -128 || message > 127 || frame >= duration)
				return false;

			event.Type = Event::Kind(kind);
			event.Message = char(message);

			if (!next.Append(frame, event))
				return false;
		}

		next.Extend(duration);
		Swap(next);

		return true;
	}
}
}
