// SPDX-License-Identifier: GPL-2.0-or-later
#include "PianoRollPlugin.h"
#include "EventBuffer.h"
#include "EventClip.h"
#include <cassert>
#include <cstdio>
#include <vector>

using namespace Spiral::Core;

static HostInfo s_Host;

static void Command(PianoRollPlugin &roll, char command)
{
	roll.GetChannelHandler()->SetCommand(command);
	roll.UpdateChannelHandler();
	roll.ExecuteCommands();
}

static void AddNote(PianoRollPlugin &roll, double start, double end, int key, float velocity)
{
	ChannelHandler *ch = roll.GetChannelHandler();
	ch->Set("Start", start);
	ch->Set("End", end);
	ch->Set("Key", key);
	ch->Set("Velocity", velocity);
	Command(roll, PianoRollPlugin::ADD_NOTE);
}

struct Played
{
	unsigned Frame;
	Event Value;
};

static std::vector<Played> Run(PianoRollPlugin &roll)
{
	roll.Execute();
	std::vector<Played> out;
	const EventBuffer *events = roll.GetEventOutput(0);

	for (unsigned i = 0; events && i < events->Count(); ++i)
	{
		Played played = {events->Frame(i), events->At(i)};
		out.push_back(played);
	}

	return out;
}

static void ClipEditsAndPlayback()
{
	PianoRollPlugin roll;
	roll.Initialise(&s_Host);
	AddNote(roll, 0.010, 0.030, 60, 0.5f);
	AddNote(roll, 0.040, 0.050, 64, 1.0f);
	std::vector<NoteSpan> notes;
	roll.Clip().Notes(notes);
	assert(notes.size() == 2);
	assert(notes[0].Start == 10 && notes[0].End == 30 && notes[0].Note.NoteNum == 60);
	assert(notes[1].Start == 40 && notes[1].End == 50 && notes[1].Note.NoteNum == 64);
	Command(roll, PianoRollPlugin::PLAY);
	assert(roll.Playing());
	const std::vector<Played> played = Run(roll);
	assert(played.size() == 4);
	assert(played[0].Frame == 10 && played[0].Value.Type == Event::NoteOn && played[0].Value.NoteNum == 60);
	assert(played[1].Frame == 30 && played[1].Value.Type == Event::NoteOff && played[1].Value.ID == played[0].Value.ID);
	assert(played[2].Frame == 40 && played[2].Value.Type == Event::NoteOn && played[2].Value.NoteNum == 64);
	assert(played[3].Frame == 50 && played[3].Value.Type == Event::NoteOff && played[3].Value.ID == played[2].Value.ID);
	assert(played[0].Value.ID != played[2].Value.ID);
	assert(!roll.Playing());
	bool ended = false;

	Sample *end = NULL;
	assert(roll.GetOutput(1, &end) && end);

	for (int n = 0; n < s_Host.BUFSIZE; ++n)
		ended = ended || (*end)[n] > 0;

	assert(ended);
	std::printf("ok   clip edits and playback\n");
}

static void LoopRestartsWithFreshIdentities()
{
	PianoRollPlugin roll;
	roll.Initialise(&s_Host);
	roll.GetChannelHandler()->Set("Loop", true);
	AddNote(roll, 0.0, 0.010, 62, 1.0f);
	ChannelHandler *ch = roll.GetChannelHandler();
	const float length = 0.020f;
	ch->Set("Length", length);
	roll.UpdateChannelHandler();
	Command(roll, PianoRollPlugin::PLAY);
	const std::vector<Played> played = Run(roll);
	assert(roll.Playing());
	assert(played.size() >= 4);
	assert(played[0].Frame == 0 && played[0].Value.Type == Event::NoteOn);
	assert(played[2].Frame == 20 && played[2].Value.Type == Event::NoteOn);
	assert(played[2].Value.ID != played[0].Value.ID);
	Command(roll, PianoRollPlugin::STOP);
	assert(!roll.Playing());
	std::printf("ok   loop restarts with fresh identities\n");
}

static void DeleteRemovesTheNote()
{
	PianoRollPlugin roll;
	roll.Initialise(&s_Host);
	AddNote(roll, 0.0, 0.010, 60, 1.0f);
	AddNote(roll, 0.020, 0.030, 61, 1.0f);
	std::vector<NoteSpan> notes;
	roll.Clip().Notes(notes);
	roll.GetChannelHandler()->Set("Index", (int)notes[0].OnEvent);
	Command(roll, PianoRollPlugin::DELETE_NOTE);
	roll.Clip().Notes(notes);
	assert(notes.size() == 1 && notes[0].Note.NoteNum == 61);
	std::printf("ok   delete removes the note\n");
}

static void RecordsTheEventInput()
{
	PianoRollPlugin roll;
	roll.Initialise(&s_Host);
	EventBuffer input;
	input.Clear(s_Host.BUFSIZE);
	Event on;
	on.Type = Event::NoteOn;
	on.ID = 7;
	on.NoteNum = 67;
	input.Append(5, on);
	Event off = on;
	off.Type = Event::NoteOff;
	input.Append(25, off);
	assert(roll.SetEventInput(0, &input));
	Command(roll, PianoRollPlugin::RECORD);
	assert(roll.Recording());
	const std::vector<Played> thru = Run(roll);
	assert(thru.size() == 2 && thru[0].Frame == 5 && thru[1].Frame == 25);
	Command(roll, PianoRollPlugin::STOP);
	std::vector<NoteSpan> notes;
	roll.Clip().Notes(notes);
	assert(notes.size() == 1);
	assert(notes[0].Note.NoteNum == 67 && notes[0].Start == 5 && notes[0].End == 25 && notes[0].Closed());
	std::printf("ok   records the event input\n");
}

static void ClipSurvivesSaveAndLoad()
{
	PianoRollPlugin roll;
	roll.Initialise(&s_Host);
	AddNote(roll, 0.010, 0.030, 60, 0.5f);
	const std::string saved = roll.Clip().Save();
	EventClip copy;
	assert(copy.Load(saved));
	std::vector<NoteSpan> notes;
	copy.Notes(notes);
	assert(notes.size() == 1 && notes[0].Start == 10 && notes[0].End == 30);
	std::printf("ok   clip survives save and load\n");
}

static void FollowsTempoAndLaunchSample()
{
	PianoRollPlugin roll;
	roll.Initialise(&s_Host);
	AddNote(roll, .008, .020, 60, 1);
	Sample tempo(s_Host.BUFSIZE), launch(s_Host.BUFSIZE);
	for (int i = 0; i < s_Host.BUFSIZE; ++i)
		tempo.Set(i, 80);
	assert(roll.SetInput(3, &tempo));
	Run(roll);
	Description saved;
	roll.Describe(saved);

	PianoRollPlugin copy;
	copy.Initialise(&s_Host);
	Description::Reader reader(saved);
	copy.Apply(reader);
	assert(!reader.Failed());

	Description positional;
	std::string error;
	assert(Description::FromJSON(saved.JSON(), positional, error));
	Description::Reader fileReader(positional);
	copy.Apply(fileReader);
	assert(!fileReader.Failed());
	for (int i = 0; i < s_Host.BUFSIZE; ++i)
	{
		tempo.Set(i, 160);
		launch.Set(i, i == 37 ? 1 : 0);
	}
	assert(copy.SetInput(3, &tempo) && copy.SetInput(1, &launch));
	const std::vector<Played> events = Run(copy);
	assert(events.size() == 2 && events[0].Frame == 41 && events[1].Frame == 47);
	std::puts("Tempo cable and saved reference place mid-block notes on exact samples");
}

static void RecordsInMusicalTime()
{
	PianoRollPlugin roll;
	roll.Initialise(&s_Host);
	Sample tempo(s_Host.BUFSIZE);
	for (int i = 0; i < s_Host.BUFSIZE; ++i)
		tempo.Set(i, 80);
	assert(roll.SetInput(3, &tempo));
	Run(roll);
	EventBuffer input;
	input.Clear(s_Host.BUFSIZE);
	assert(roll.SetEventInput(0, &input));
	Command(roll, PianoRollPlugin::RECORD);
	for (int i = 0; i < s_Host.BUFSIZE; ++i)
		tempo.Set(i, 160);
	Event note;
	note.Type = Event::NoteOn;
	note.ID = 1;
	note.NoteNum = 60;
	assert(input.Append(5, note));
	note.Type = Event::NoteOff;
	assert(input.Append(10, note));
	Run(roll);
	Command(roll, PianoRollPlugin::STOP);
	std::vector<NoteSpan> notes;
	roll.Clip().Notes(notes);
	assert(notes.size() == 1 && notes[0].Start == 10 && notes[0].End == 20);
}

static void SavedCueUsesTheClipRate()
{
	HostInfo sourceHost = s_Host;
	sourceHost.SAMPLERATE = 48000;
	PianoRollPlugin source;
	source.Initialise(&sourceHost);
	AddNote(source, 1.0, 1.1, 60, 1);
	Description saved;
	source.Describe(saved);

	HostInfo playbackHost = s_Host;
	playbackHost.SAMPLERATE = 44100;
	PianoRollPlugin roll;
	roll.Initialise(&playbackHost);
	Description::Reader reader(saved);
	roll.Apply(reader);
	assert(!reader.Failed());
	Command(roll, PianoRollPlugin::PLAY);
	for (unsigned frame = 0; frame <= 44100; frame += playbackHost.BUFSIZE)
	{
		const std::vector<Played> events = Run(roll);
		if (frame + playbackHost.BUFSIZE <= 44100)
			assert(events.empty());
		else
			assert(events.size() == 1 && events[0].Frame + frame == 44100);
	}
	std::puts("Unclocked 48 kHz patch plays its one-second cue at frame 44100");
}

int main()
{
	s_Host.BUFSIZE = 64;
	s_Host.ROLLING = true;
	s_Host.SAMPLERATE = 1000;
	FollowsTempoAndLaunchSample();
	RecordsInMusicalTime();
	SavedCueUsesTheClipRate();
	ClipEditsAndPlayback();
	LoopRestartsWithFreshIdentities();
	DeleteRemovesTheNote();
	RecordsTheEventInput();
	ClipSurvivesSaveAndLoad();

	return 0;
}
