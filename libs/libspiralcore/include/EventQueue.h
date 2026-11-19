// Copyright (C) 2004 David Griffiths <dave@pawfal.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifndef SSM_CORE_EVENT_QUEUE_H
#define SSM_CORE_EVENT_QUEUE_H
#include "Event.h"

namespace Spiral
{
namespace Core
{
	// Single-owner, bounded ordered ring. Cross-thread publication belongs to
	// the existing mailboxes, not this container. Equal-frame order is stable.
	class EventQueue
	{
	public:
		enum { Capacity = 256 };
		EventQueue(): head(0), count(0) {}
		void Clear() { head = count = 0; }
		bool Add(const Event &event)
		{
			if (count == Capacity)
				return false;
			if (count)
			{
				const AudioStamp &last = events[(head + count - 1) % Capacity].TimeStamp;
				if (last.Generation != event.TimeStamp.Generation || last.Frame > event.TimeStamp.Frame)
					return false;
			}
			events[(head + count++) % Capacity] = event;
			return true;
		}
		// Half-open [from, till): boundary events are delivered exactly once.
		// Passed events and earlier generations are discarded on forward seeks.
		bool Get(const AudioStamp &from, const AudioStamp &till, Event &event)
		{
			if (from.Generation != till.Generation || till.Frame <= from.Frame)
				return false;
			while (count)
			{
				const Event &next = events[head];
				if (next.TimeStamp.Generation > from.Generation ||
					(next.TimeStamp.Generation == from.Generation && next.TimeStamp.Frame >= till.Frame))
					return false;
				const bool due = next.TimeStamp.Generation == from.Generation && next.TimeStamp.Frame >= from.Frame;
				event = next;
				head = (head + 1) % Capacity;
				--count;
				if (due)
					return true;
			}
			return false;
		}

	private:
		Event events[Capacity];
		unsigned head, count;
	};
}
}
#endif
