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

#ifndef SSM_EVENT_VOICES_H
#define SSM_EVENT_VOICES_H
#include "Event.h"
#include <algorithm>
#include <cmath>
#include <stdint.h>

namespace Spiral
{
namespace Core
{
	class EventVoices
	{
	public:
		struct Voice
		{
			int ID, Channel, Key;
			float Frequency, Level;
			uint64_t Age;
			bool Held, Retrigger;
			// Its note ended since its gate was last read: a note taking
			// the voice then starts with the gate down, as a new note must.
			bool Released;

			Voice():
			    ID(-1),
			    Channel(0),
			    Key(0),
			    Frequency(440),
			    Level(0),
			    Age(0),
			    Held(false),
			    Retrigger(false),
			    Released(false)
			{
			}
		};

		EventVoices():
		    count(1),
		    serial(0)
		{
			Reset();
		}

		void Configure(unsigned voices)
		{
			voices = std::max(1u, std::min(256u, voices));
			if (count != voices)
			{
				count = voices;
				Reset();
			}
		}

		void Reset()
		{
			for (unsigned i = 0; i < 256; ++i)
				lanes[i] = Voice();
			for (unsigned c = 0; c < 16; ++c)
			{
				sustain[c] = false;
				bend[c] = 1;
			}
			serial = 0;
		}

		unsigned Count() const
		{
			return count;
		}

		const Voice &Get(unsigned lane) const
		{
			return lanes[lane];
		}

		float Pitch(unsigned lane) const
		{
			return lanes[lane].Frequency * bend[lanes[lane].Channel];
		}

		float Gate(unsigned lane)
		{
			Voice &voice = lanes[lane];
			const float result = voice.Retrigger ? 0 : voice.Level;
			voice.Retrigger = false;
			voice.Released = false;
			return result;
		}

		// A note may name the voice it takes, as the Matrix does to keep
		// its highest notes; otherwise it takes the first free voice, or
		// else the oldest.
		void Apply(const Event &event, int lane = -1)
		{
			if (!event.Valid())
				return;
			if (event.Type == Event::Reset)
			{
				Reset();
				return;
			}
			const unsigned channel = event.Channel;
			if (event.Type == Event::PitchBend)
				bend[channel] = std::pow(2.0f, event.Value * (2.0f / 12.0f));
			if (event.Type == Event::Controller)
			{
				if (event.Parameter == 64)
					sustain[channel] = event.Value >= .5f;
				if (event.Parameter == 121)
				{
					sustain[channel] = false;
					bend[channel] = 1;
				}
				for (unsigned i = 0; i < count; ++i)
				{
					Voice &voice = lanes[i];
					if (voice.Channel != int(channel))
						continue;
					if (event.Parameter == 120 || event.Parameter == 123)
						voice.Held = false;
					if (event.Parameter == 120 || (!sustain[channel] && !voice.Held))
					{
						voice.Released = voice.Released || voice.Level != 0;
						voice.Level = 0;
					}
				}
			}
			if (event.Type == Event::NoteOff)
				for (unsigned i = 0; i < count; ++i)
					if (lanes[i].Channel == event.Channel && lanes[i].ID == event.ID)
					{
						lanes[i].Held = false;
						if (!sustain[channel])
						{
							lanes[i].Released = lanes[i].Released || lanes[i].Level != 0;
							lanes[i].Level = 0;
						}
					}
			if (event.Type != Event::NoteOn)
				return;
			unsigned selected = 0;
			for (unsigned i = 0; i < count && lane < 0; ++i)
			{
				if (lanes[i].Level == 0)
				{
					selected = i;
					break;
				}
				if (lanes[i].Age < lanes[selected].Age)
					selected = i;
			}
			if (lane >= 0 && unsigned(lane) < count)
				selected = unsigned(lane);
			Voice &voice = lanes[selected];
			voice.Retrigger = voice.Level != 0 || voice.Released;
			voice.ID = event.ID;
			voice.Channel = event.Channel;
			voice.Key = event.NoteNum;
			voice.Frequency = event.Frequency;
			voice.Level = event.Volume;
			voice.Held = true;
			voice.Age = ++serial;
		}

	private:
		Voice lanes[256];
		bool sustain[16];
		float bend[16];
		unsigned count;
		uint64_t serial;
	};
}
}
#endif
