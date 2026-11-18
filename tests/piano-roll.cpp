// SPDX-License-Identifier: GPL-2.0-or-later
#include "PianoRollPlugin.h"
#include "EventBuffer.h"
#include "EventClip.h"
#include "PatchFile.h"
#include "PatchProject.h"
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

// Fifteen minutes at the event input, including overlapping same-key
// notes on two channels, differing release times, and controller changes.
static void LongTakeRoundTrip(const char *destination)
{
	HostInfo host = s_Host;
	host.SAMPLERATE = 48000;
	host.BUFSIZE = 256;
	const unsigned chords = 6000, stride = 7200;
	EventClip expected;
	expected.Clear(host.SAMPLERATE);
	for (unsigned i = 0; i < chords; ++i)
	{
		Event chord[3];
		const unsigned start = i * stride;
		for (unsigned n = 0; n < 3; ++n)
		{
			chord[n].ID = int(3 * i + n);
			chord[n].Channel = n % 2;
			chord[n].NoteNum = 48 + i % 24;
			chord[n].Volume = .25f * (n + 1);
			assert(expected.Append(start, chord[n]));
		}
		if (i % 8 == 0)
		{
			Event cc;
			cc.Type = Event::Controller;
			cc.Parameter = 11;
			cc.Value = (i % 32) / 32.f;
			assert(expected.Append(start + 123, cc));
		}
		for (unsigned n = 0; n < 3; ++n)
		{
			Event off = chord[2 - n];
			off.Type = Event::NoteOff;
			assert(expected.Append(start + 1600 + n * 211, off));
		}
	}
	expected.Extend(chords * stride);
	const std::string reference = expected.Save();
	Spumoni::DiskFolder folder;
	std::string error;
	assert(folder.Create("ssm-piano-roll-", error));
	const std::string path = destination ? destination : folder.Root() + "/take.ssmp";

	{
		PianoRollPlugin roll;
		roll.Initialise(&host);
		EventBuffer in;
		assert(roll.SetEventInput(0, &in));
		Command(roll, PianoRollPlugin::RECORD);
		unsigned fed = 0;
		for (unsigned frame = 0; frame < expected.Length(); frame += host.BUFSIZE)
		{
			in.Clear(host.BUFSIZE);
			while (fed < expected.Count() && expected.FrameAt(fed) < frame + host.BUFSIZE)
			{
				assert(in.Append(expected.FrameAt(fed) - frame, expected.At(fed)));
				++fed;
			}
			roll.Execute();
		}
		assert(fed == expected.Count() && roll.Recording());
		Command(roll, PianoRollPlugin::STOP);
		assert(roll.Clip().Save() == reference);

		PatchDescription patch;
		DeviceDescription &device = patch.AddObject().AddDevice();
		device.SetValue("patch_id", "1");
		device.SetValue("id", "288");
		device.SetValue("name", "PianoRoll");
		Description description;
		roll.Describe(description);
		Spiral::File::PatchFile::Properties(description, device);
		std::string json;
		assert(Spiral::File::PatchFile::Text(patch, json, error));
		Spiral::File::Project writer("");
		writer.Patch().Set(json);
		assert(writer.SaveAs(path, error));
	}

	Spiral::File::Project reader(path);
	assert(reader.OpenPackage("", error));
	PatchDescription reopened;
	assert(Spiral::File::PatchFile::ReadText(reader.Patch().Bytes(), reopened, error));
	const DeviceDescription *device = reopened.FindDevice("1");
	assert(device);
	Description saved;
	Spiral::File::PatchFile::Describe(*device, saved);
	PianoRollPlugin restored;
	restored.Initialise(&host);
	Description::Reader state(saved);
	restored.Apply(state);
	assert(!state.Failed() && restored.Clip().Save() == reference);

	std::vector<NoteSpan> notes;
	restored.Clip().Notes(notes);
	assert(notes.size() == 3 * chords);
	for (unsigned i = 0; i < notes.size(); ++i)
	{
		assert(notes[i].Closed());
		assert(restored.Clip().At(notes[i].OffEvent).ID == notes[i].Note.ID);
	}
	restored.UpdateChannelHandler();
	std::vector<double> rows;
	unsigned long revision = 0;
	assert(restored.GetChannelHandler()->ReadTable("NoteRows", revision, rows));
	assert(rows.size() == notes.size() * PianoRollPlugin::NOTE_CELLS);
	for (unsigned i = 0; i < notes.size(); ++i)
	{
		const double *row = &rows[i * PianoRollPlugin::NOTE_CELLS];
		assert(row[PianoRollPlugin::NOTE_INDEX] == notes[i].OnEvent);
		assert(row[PianoRollPlugin::NOTE_END] == double(notes[i].End) / host.SAMPLERATE);
	}
	assert(!restored.GetChannelHandler()->ReadTable("NoteRows", revision, rows));
	std::printf("900-second take: %u note-ons, %u note-offs, %u controllers, %u rows. All saved event fields and frames match after reopening %s\n",
		3 * chords, 3 * chords, (chords + 7) / 8, (unsigned)notes.size(), path.c_str());
}

static void FullTakeRetainsEveryAcceptedEvent()
{
	PianoRollPlugin roll;
	roll.Initialise(&s_Host);
	EventBuffer input;
	assert(roll.SetEventInput(0, &input));
	Command(roll, PianoRollPlugin::RECORD);
	Event cc;
	cc.Type = Event::Controller;
	cc.Parameter = 11;
	for (unsigned written = 0; written <= EventClip::MaxEvents && roll.Recording();)
	{
		input.Clear(s_Host.BUFSIZE);
		for (int n = 0; n < s_Host.BUFSIZE && written <= EventClip::MaxEvents; ++n, ++written)
		{
			Event event = cc;
			if (written == EventClip::MaxEvents - 1)
			{
				event.Type = Event::NoteOn;
				event.ID = 123;
			}
			assert(input.Append(n, event));
		}
		roll.Execute();
	}
	assert(!roll.Recording() && roll.Clip().Count() == EventClip::MaxEvents);
	assert(roll.Clip().At(EventClip::MaxEvents - 1).ID == 123);
	roll.UpdateChannelHandler();
	char status[80] = {};
	assert(roll.GetChannelHandler()->Peek("Record Status", 0, status, sizeof(status)));
	assert(std::string(status).find("Clip full") != std::string::npos);
	std::puts("Full take retains all accepted events and publishes its capacity status");
}

int main(int argc, char **argv)
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
	FullTakeRetainsEveryAcceptedEvent();
	LongTakeRoundTrip(argc == 2 ? argv[1] : NULL);

	return 0;
}
