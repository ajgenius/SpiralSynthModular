// SPDX-License-Identifier: GPL-2.0-or-later
#include "EventVoices.h"
#include <cassert>

using namespace Spiral::Core;

static Event Note(int id)
{
	Event note;
	note.ID = id;
	note.NoteNum = 60;
	return note;
}

static void Release(EventVoices &voices, Event note)
{
	note.Type = Event::NoteOff;
	voices.Apply(note);
}

static void Pedal(EventVoices &voices, bool down)
{
	Event cc;
	cc.Type = Event::Controller;
	cc.Parameter = 64;
	cc.Value = down ? 1 : 0;
	voices.Apply(cc);
}

static void Articulation()
{
	EventVoices voices;
	Event first = Note(1);
	voices.Apply(first);
	assert(voices.Gate(0) == 1);

	// A held note continues across a frame boundary and a pitch change.
	Event bend = first;
	bend.Type = Event::PitchBend;
	bend.Value = 1;
	voices.Apply(bend);
	assert(voices.Pitch(0) > first.Frequency && voices.Gate(0) == 1);

	Release(voices, first);
	Event second = Note(2);
	voices.Apply(second);
	assert(voices.Gate(0) == 0);
	assert(voices.Gate(0) == 1);

	Pedal(voices, true);
	Release(voices, second);
	assert(voices.Gate(0) == 1);
	Pedal(voices, false);
	voices.Apply(Note(3));
	assert(voices.Gate(0) == 0);
	assert(voices.Gate(0) == 1);

	// A release already rendered supplies the edge itself.
	Release(voices, Note(3));
	assert(voices.Gate(0) == 0);
	voices.Apply(Note(4));
	assert(voices.Gate(0) == 1);
}

static void ReleaseTails()
{
	EventVoices voices;
	voices.Configure(3);
	voices.Apply(Note(1));
	Release(voices, Note(1));
	voices.Apply(Note(2));
	assert(voices.Get(1).ID == 2);
	voices.Apply(Note(3));
	assert(voices.Get(2).ID == 3);
	voices.Apply(Note(4));
	assert(voices.Get(0).ID == 4);

	// Lanes 0 and 2 become idle in the reverse of their attack order.
	Release(voices, Note(4));
	Release(voices, Note(3));
	voices.Apply(Note(5));
	assert(voices.Get(0).ID == 5);
	voices.Apply(Note(6));
	assert(voices.Get(2).ID == 6);

	// The oldest sounding note is pedal-held. Its late off must leave the replacement alone.
	Pedal(voices, true);
	Release(voices, Note(2));
	assert(voices.Gate(1) == 1);
	voices.Apply(Note(7));
	assert(voices.Get(1).ID == 7);
	assert(voices.Gate(1) == 0);
	Release(voices, Note(2));
	Pedal(voices, false);
	assert(voices.Gate(1) == 1 && voices.Get(1).Held);
}

int main()
{
	Articulation();
	ReleaseTails();

	EventVoices mapped;
	mapped.Configure(3);
	mapped.Apply(Note(1), 2);
	assert(mapped.Get(2).ID == 1 && mapped.Gate(2) == 1);
}
