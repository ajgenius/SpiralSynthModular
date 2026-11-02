// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SSM_EVENT_BUFFER_H
#define SSM_EVENT_BUFFER_H
#include "Event.h"
namespace Spiral
{
namespace Core
{
	class EventBuffer
	{
	public:
		enum { Capacity = 1024 };
		EventBuffer(): count(0), frames(0), failed(false) {}
		void Clear(unsigned samples) { count = 0; frames = samples; failed = false; }
		bool Append(unsigned frame, const Event &event)
		{
			if (failed)
				return false;
			if (!event.Valid() || frame >= frames || count == Capacity || (count && frame < offsets[count - 1]))
			{
				count = 0;
				if (frames)
				{
					Event reset;
					reset.Type = Event::Reset;
					Append(0, reset);
				}
				failed = true;
				return false;
			}
			offsets[count] = frame;
			events[count++] = event;
			return true;
		}
		unsigned Count() const { return count; }
		unsigned Frame(unsigned index) const { return offsets[index]; }
		const Event &At(unsigned index) const { return events[index]; }
		bool Failed() const { return failed; }
	private:
		Event events[Capacity];
		unsigned offsets[Capacity], count, frames;
		bool failed;
	};
}
}
#endif
