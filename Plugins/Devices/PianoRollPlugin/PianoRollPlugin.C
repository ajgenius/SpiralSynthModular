// SPDX-License-Identifier: GPL-2.0-or-later
#include "PianoRollPlugin.h"
#include "EventBuffer.h"
#include "SpiralIcon.xpm"
#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstring>

#include <config.h>

using namespace Spiral::Core;

extern "C"
{
	const char *SpiralPlugin_GetHostVersion()
	{
		return PACKAGE_VERSION;
	}

	const char *SpiralPlugin_GetHostABI()
	{
		return SSM_HOST_ABI;
	}

	SpiralPlugin *SpiralPlugin_CreateInstance()
	{
		return new PianoRollPlugin;
	}

	int SpiralPlugin_GetType()
	{
		return SPIRAL_PLUGIN_TYPE_DEVICE;
	}

	const char **SpiralPlugin_GetIcon()
	{
		return SpiralIcon_xpm;
	}
}

PianoRollPlugin::PianoRollPlugin():
	m_Saved(Property::WriteOnly, ""),
	m_Loop(Property::WriteOnly, false),
	m_Thru(Property::WriteOnly, true),
	m_PlayingState(Property::ReadOnly, false),
	m_RecordingState(Property::ReadOnly, false),
	m_Length(DefaultLinearFlags, 0, 0, 3600, 0.01f, 1),
	m_Position(Property::ReadOnly, 0, 0, 86400, 0.01f, 1),
	m_NoteCount(Property::ReadOnly, 0, 0, EventClip::MaxEvents, 1, 1),
	m_LiveConnected(false),
	m_OutputQueue(new Outgoing[QUEUED_EVENTS]),
	m_Queued(0),
	m_QueueHead(0),
	m_BlockFrame(0),
	m_Overflow(false),
	m_Playing(false),
	m_Recording(false),
	m_LaunchHigh(false),
	m_StopHigh(false),
	m_Phase(0),
	m_RecordStart(0),
	m_OriginalTempo(0),
	m_TempoPaused(false),
	m_Next(0),
	m_NextID(0),
	m_IDWrapped(false),
	m_Rate(48000)
{
	std::memset(m_Pedal, 0, sizeof(m_Pedal));
	m_Tails[Live] = m_Tails[Playback] = 0;
	m_Held[Playback].reserve(MAX_HELD);
	m_Held[Live].reserve(MAX_HELD);
	std::memset(&m_Args, 0, sizeof(m_Args));
	RegisterSharedProperty(m_Saved, "Clip", "The recorded and edited notes");
	RegisterSharedProperty(m_Loop, "Loop", "Repeat the clip");
	RegisterSharedProperty(m_Thru, "Thru", "Forward incoming events");
	RegisterSharedProperty(m_Length, "Length", "Clip length in seconds, 0 to end with the last note");
	RegisterSharedProperty(m_PlayingState, "Playing", "Playing now");
	RegisterSharedProperty(m_RecordingState, "Recording", "Recording now");
	RegisterSharedProperty(m_Position, "Position", "Seconds played");
	RegisterSharedProperty(m_NoteCount, "Notes", "Notes in the clip");
	m_AudioCH->Register("Index", &m_Args.Index, ChannelHandler::INPUT);
	m_AudioCH->Register("Key", &m_Args.Key, ChannelHandler::INPUT);
	m_AudioCH->Register("Velocity", &m_Args.Velocity, ChannelHandler::INPUT);
	m_AudioCH->Register("Start", &m_Args.Start, ChannelHandler::INPUT);
	m_AudioCH->Register("End", &m_Args.End, ChannelHandler::INPUT);
	m_AudioCH->RegisterTable("NoteRows");
	std::memset(m_RecordStatus, 0, sizeof(m_RecordStatus));
	m_AudioCH->RegisterData("Record Status", ChannelHandler::OUTPUT, m_RecordStatus, sizeof(m_RecordStatus), 's');
	std::vector<std::string> none, note, edit, index, position;
	note.push_back("Start");
	note.push_back("End");
	note.push_back("Key");
	note.push_back("Velocity");
	edit.push_back("Index");
	edit.insert(edit.end(), note.begin(), note.end());
	index.push_back("Index");
	position.push_back("Start");
	m_AudioCH->RegisterCommand("PLAY", PLAY, none);
	m_AudioCH->RegisterCommand("STOP", STOP, none);
	m_AudioCH->RegisterCommand("RECORD", RECORD, none);
	m_AudioCH->RegisterCommand("CLEAR", CLEAR, none);
	m_AudioCH->RegisterCommand("ADD_NOTE", ADD_NOTE, note);
	m_AudioCH->RegisterCommand("EDIT_NOTE", EDIT_NOTE, edit);
	m_AudioCH->RegisterCommand("DELETE_NOTE", DELETE_NOTE, index);
	m_AudioCH->RegisterCommand("SET_POSITION", SET_POSITION, position);
	m_Version = 0;
	SetUnsigned(DeviceVersion, 0);
	m_PluginInfo.Name = "PianoRoll";
	m_PluginInfo.Width = 420;
	m_PluginInfo.Height = 260;
	m_PluginInfo.NumInputs = 4;
	m_PluginInfo.NumOutputs = 2;
	m_PluginInfo.PortTips.push_back("Events");
	m_PluginInfo.PortTips.push_back("Launch CV");
	m_PluginInfo.PortTips.push_back("Stop CV");
	m_PluginInfo.PortTips.push_back("Tempo");
	m_PluginInfo.PortTips.push_back("Events");
	m_PluginInfo.PortTips.push_back("End CV");
	m_Clip.Clear(m_Rate);
}

PianoRollPlugin::~PianoRollPlugin()
{
	delete[] m_OutputQueue;
}

PluginInfo &PianoRollPlugin::Initialise(const HostInfo *Host)
{
	PluginInfo &info = SpiralPlugin::Initialise(Host);
	m_PluginInfo.PortTypes.assign(m_PluginInfo.NumInputs + m_PluginInfo.NumOutputs, Sample::AUDIO);
	SetInPortType(m_PluginInfo, 0, Sample::EVENT);
	SetOutPortType(m_PluginInfo, 0, Sample::EVENT);
	m_Rate = Host && Host->SAMPLERATE > 0 ? (unsigned)Host->SAMPLERATE : 48000;

	if (!m_Clip.Count())
		m_Clip.Clear(m_Rate);

	Published();

	return info;
}

uint64_t PianoRollPlugin::Frames(double seconds) const
{
	return seconds > 0 ? (uint64_t)(seconds * m_Clip.Rate() + 0.5) : 0;
}

uint64_t PianoRollPlugin::ClipLength() const
{
	const uint64_t set = Frames(AsFloat(m_Length));

	return set ? set : m_Clip.Length();
}

void PianoRollPlugin::Queue(unsigned frame, const Event &value)
{
	if (m_Queued == QUEUED_EVENTS)
	{
		m_Overflow = true;
		return;
	}

	Outgoing &out = m_OutputQueue[(m_QueueHead + m_Queued) % QUEUED_EVENTS];
	out.Frame = m_BlockFrame + frame;
	++m_Queued;
	out.Value = value;
}

int PianoRollPlugin::NextID()
{
	// Only a counter that has wrapped can encounter an ID still in use.
	if (m_NextID < INT_MAX && !m_IDWrapped)
		return ++m_NextID;

	m_IDWrapped = true;
	for (;;)
	{
		m_NextID = m_NextID == INT_MAX ? 1 : m_NextID + 1;
		bool used = false;
		for (unsigned s = 0; s < 2; ++s)
			for (unsigned i = 0; i < m_Held[s].size(); ++i)
				used = used || m_Held[s][i].Off.ID == m_NextID;
		if (!used)
			return m_NextID;
	}
}

float PianoRollPlugin::OutputPedal(int channel) const
{
	return std::max(m_Pedal[Playback][channel], m_Pedal[Live][channel]);
}

void PianoRollPlugin::QueuePedal(unsigned frame, int channel, float value)
{
	Event event;
	event.Type = Event::Controller;
	event.Channel = channel;
	event.Parameter = 64;
	event.Value = value;
	Queue(frame, event);
}

void PianoRollPlugin::QueueRelease(unsigned frame, const Event &off)
{
	const float pedal = OutputPedal(off.Channel);
	if (pedal >= .5f)
		QueuePedal(frame, off.Channel, 0);
	Queue(frame, off);
	if (pedal >= .5f)
		QueuePedal(frame, off.Channel, pedal);
}

// The native event receiver has at most 256 sounding lanes. Retire the
// oldest pedal-held release when a long pedal gesture fills those lanes.
// Physically held keys keep their mappings until their own releases arrive.
void PianoRollPlugin::ReleaseTail(Source source, unsigned frame)
{
	std::vector<Held> &notes = m_Held[source];
	for (unsigned i = 0; i < notes.size(); ++i)
	{
		if (notes[i].KeyDown)
			continue;
		QueueRelease(frame, notes[i].Off);
		notes.erase(notes.begin() + i);
		--m_Tails[source];
		return;
	}
}

// MIDI has a single damper per channel. Keep each source's pedal-held keys
// down on the merged wire. This lets one source release without lifting the
// other's notes, including notes whose physical keys are already up.
void PianoRollPlugin::ReleaseNotes(Source source, unsigned frame, int channel, bool keysToo)
{
	std::vector<Held> &notes = m_Held[source];
	const float pedal = OutputPedal(channel);
	bool releasing = false;
	unsigned keep = 0;
	for (unsigned i = 0; i < notes.size(); ++i)
	{
		if (notes[i].Off.Channel == channel && (keysToo || !notes[i].KeyDown))
		{
			if (!releasing && pedal >= .5f)
				QueuePedal(frame, channel, 0);
			releasing = true;
			Queue(frame, notes[i].Off);
			if (!notes[i].KeyDown)
				--m_Tails[source];
		}
		else
		{
			if (keep != i)
				notes[keep] = notes[i];
			++keep;
		}
	}
	notes.resize(keep);
	if (releasing && pedal >= .5f)
		QueuePedal(frame, channel, pedal);
}

void PianoRollPlugin::Pedal(Source source, unsigned frame, const Event &event)
{
	const int channel = event.Channel;
	const float previous = OutputPedal(channel);
	m_Pedal[source][channel] = event.Value;
	Event cc = event;
	cc.Value = OutputPedal(channel);
	if (cc.Value != previous || cc.Value == event.Value)
		Queue(frame, cc);
	if (event.Value < .5f)
		ReleaseNotes(source, frame, channel, false);
}

void PianoRollPlugin::Emit(Source source, unsigned frame, const Event &event)
{
	if (m_Overflow)
		return;

	std::vector<Held> &notes = m_Held[source];
	Event value = event;
	if (event.Type == Event::NoteOn)
	{
		if (notes.size() == notes.capacity())
		{
			if (!m_Tails[source])
				return;
			ReleaseTail(source, frame);
		}
		value.ID = NextID();
		Held note;
		note.InputID = event.ID;
		note.KeyDown = true;
		note.Off = value;
		note.Off.Type = Event::NoteOff;
		notes.push_back(note);
	}
	else if (event.Type == Event::NoteOff)
	{
		// Clip pairing and live MIDI both release reused identities in arrival order.
		unsigned i = 0;
		while (i < notes.size() && !(notes[i].KeyDown && notes[i].Off.Channel == event.Channel &&
					     notes[i].InputID == event.ID))
			++i;
		if (i == notes.size())
			return;
		value.ID = notes[i].Off.ID;
		if (m_Pedal[source][event.Channel] >= .5f)
		{
			notes[i].KeyDown = false;
			notes[i].Off = value;
			if (++m_Tails[source] > MAX_PEDAL_TAILS)
				ReleaseTail(source, frame);
			return;
		}
		QueueRelease(frame, value);
		notes.erase(notes.begin() + i);
		return;
	}
	else if (event.Type == Event::Controller && event.Parameter == 64)
	{
		Pedal(source, frame, event);
		return;
	}
	else if (event.Type == Event::Controller && (event.Parameter == 120 || event.Parameter == 123))
	{
		if (event.Parameter == 123 && m_Pedal[source][event.Channel] >= .5f)
		{
			for (unsigned i = 0; i < notes.size(); ++i)
				if (notes[i].Off.Channel == event.Channel && notes[i].KeyDown)
				{
					notes[i].KeyDown = false;
					++m_Tails[source];
				}
			while (m_Tails[source] > MAX_PEDAL_TAILS)
				ReleaseTail(source, frame);
		}
		else
			ReleaseNotes(source, frame, event.Channel, true);
		return;
	}
	else if (event.Type == Event::Controller && event.Parameter == 121)
	{
		Queue(frame, event);
		m_Pedal[source][event.Channel] = 0;
		const float pedal = OutputPedal(event.Channel);
		if (pedal != 0)
			QueuePedal(frame, event.Channel, pedal);
		ReleaseNotes(source, frame, event.Channel, false);
		return;
	}
	else if (event.Type == Event::Reset)
	{
		// An upstream reset can clear the whole receiver during solo thru.
		// During playback only the originating stream owns its releases.
		if (source == Live && (!m_Playing || m_Recording))
		{
			notes.clear();
			m_Tails[source] = 0;
			std::memset(m_Pedal[source], 0, sizeof(m_Pedal[source]));
			Queue(frame, event);
			return;
		}
		ReleaseAll(source, frame);
		return;
	}
	Queue(frame, value);
}

void PianoRollPlugin::ReleaseAll(Source source, unsigned frame)
{
	for (int channel = 0; channel < 16; ++channel)
	{
		if (m_Pedal[source][channel] != 0)
		{
			const float previous = OutputPedal(channel);
			m_Pedal[source][channel] = 0;
			const float pedal = OutputPedal(channel);
			if (pedal != previous)
				QueuePedal(frame, channel, pedal);
		}
		if (!m_Held[source].empty())
			ReleaseNotes(source, frame, channel, true);
	}
}

void PianoRollPlugin::Start(bool record)
{
	// Play starts from the top. Record starts where the roll is: where it
	// last stopped, or where it has got to if it is playing.
	if (!m_Playing && !record)
	{
		m_Clock.Reset();
		m_Phase = 0;
		m_Next = 0;
	}

	m_Playing = true;

	if (record && !m_Recording)
	{
		// The roll under a take is not heard; what was sounding stops.
		ReleaseAll(Playback, 0);
		m_Recording = true;
		m_RecordStart = m_Phase;
		m_Take.Clear(m_Clip.Rate());
		m_RecordStatus[0] = 0;
	}
}

void PianoRollPlugin::Stop(unsigned frame)
{
	ReleaseAll(Playback, frame);

	if (m_Recording && !MergeRecording())
		std::snprintf(m_RecordStatus, sizeof(m_RecordStatus), "Take exceeds capacity. Original clip retained.");

	m_Playing = false;
	m_Recording = false;
}

void PianoRollPlugin::InputEvent(const Event &event, unsigned frame, bool record)
{
	if (record && event.Type != Event::Reset && !m_Take.Append(m_Phase, event))
	{
		std::snprintf(m_RecordStatus, sizeof(m_RecordStatus), "Recording stopped at clip capacity");
		Stop(frame);
	}

	if (ValueAsBoolean(m_Thru))
		Emit(Live, frame, event);
}

// The take replaces what it ran over, from where Record began to where it
// stopped: the roll before it stays, with notes sounding into it ended there,
// and the roll after it stays, with notes it stopped in the middle of starting
// again there. A take that never ran leaves the roll as it was.
bool PianoRollPlugin::MergeRecording()
{
	if (m_Phase == m_RecordStart && !m_Take.Count())
		return true;

	// The take already owns every input event. Give an empty roll that
	// storage directly. At capacity, clip end releases any still-held
	// notes even if their synthetic releases no longer fit in the clip.
	if (!m_Clip.Count())
	{
		if (!m_Take.AppendFrom(m_Clip, m_Phase))
			std::snprintf(m_RecordStatus, sizeof(m_RecordStatus), "Clip full. Held notes release at the end.");
		m_Take.Extend(m_Phase);
		m_Clip.Swap(m_Take);
		Published();
		return true;
	}

	EventClip merged;
	if (!merged.CopyBefore(m_Clip, m_RecordStart))
		return false;

	// Keep punched note identities clear of the original performance.
	int base = -1;
	for (unsigned i = 0; i < m_Clip.Count(); ++i)
		if (m_Clip.At(i).Type == Event::NoteOn || m_Clip.At(i).Type == Event::NoteOff)
			base = std::max(base, m_Clip.At(i).ID);

	for (unsigned i = 0; i < m_Take.Count(); ++i)
	{
		Event event = m_Take.At(i);
		if (event.Type == Event::NoteOn || event.Type == Event::NoteOff)
		{
			const int64_t id = int64_t(base) + 1 + event.ID;
			if (id > INT_MAX)
				return false;
			event.ID = (int)id;
		}
		if (!merged.Append(m_Take.FrameAt(i), event))
			return false;
	}

	if (!merged.AppendFrom(m_Clip, m_Phase))
		return false;
	merged.Extend(m_Phase);
	m_Clip.Swap(merged);
	Published();
	return true;
}

void PianoRollPlugin::Published()
{
	std::vector<NoteSpan> notes;
	m_Clip.Notes(notes);
	std::vector<double> cells(notes.size() * NOTE_CELLS);
	for (size_t i = 0; i < notes.size(); ++i)
	{
		const NoteSpan &note = notes[i];
		double *row = &cells[i * NOTE_CELLS];
		row[NOTE_START] = double(note.Start) / m_Clip.Rate();
		row[NOTE_END] = double(note.End) / m_Clip.Rate();
		row[NOTE_KEY] = note.Note.NoteNum;
		row[NOTE_VELOCITY] = note.Note.Volume;
		row[NOTE_INDEX] = note.OnEvent;
		row[NOTE_CHANNEL] = note.Note.Channel;
	}
	m_AudioCH->PublishTable("NoteRows", cells);
	SetUnsigned(m_NoteCount, notes.size());
	std::string text = m_Clip.Save();
	std::replace(text.begin(), text.end(), '\n', ' ');
	m_Saved.Value.swap(text);
}

void PianoRollPlugin::Execute()
{
	EventBuffer *out = GetEventOutput(0);
	const EventBuffer *in = GetEventInput(0);
	const unsigned frames = (unsigned)m_HostInfo->BUFSIZE;

	const bool liveConnected = in && ValueAsBoolean(m_Thru);
	if (m_LiveConnected && !liveConnected)
		ReleaseAll(Live, 0);
	m_LiveConnected = liveConnected;

	const double bpm = frames && InputExists(3) ? GetInput(3, 0) : 0;
	const bool follows = ClipTempo::Valid(bpm);
	if (follows && !m_OriginalTempo)
		m_OriginalTempo = bpm;
	const bool paused = follows && !m_HostInfo->ROLLING;
	if (paused && !m_TempoPaused && m_Playing)
		ReleaseAll(Playback, 0);
	m_TempoPaused = paused;

	const double advance = double(m_Clip.Rate()) / m_Rate * (follows ? bpm / m_OriginalTempo : 1.0);
	const uint64_t length = ClipLength();
	unsigned incoming = 0;
	for (unsigned n = 0; n < frames; ++n)
	{
		SetOutput(1, n, 0);
		const bool launch = InputExists(1) && GetInput(1, n) > 0.1f;
		const bool stop = InputExists(2) && GetInput(2, n) > 0.1f;
		if (stop && !m_StopHigh && m_Playing)
			Stop(n);
		else if (launch && !m_LaunchHigh)
		{
			if (m_Playing)
				Stop(n);
			Start(false);
		}
		m_LaunchHigh = launch;
		m_StopHigh = stop;

		while (in && incoming < in->Count() && in->Frame(incoming) == n)
			InputEvent(in->At(incoming++), n, m_Recording && !paused);
		if (!m_Playing || paused)
			continue;

		if (!m_Recording && length && m_Phase >= length)
		{
			while (m_Next < m_Clip.Count() && m_Clip.FrameAt(m_Next) <= length)
				Emit(Playback, n, m_Clip.At(m_Next++));
			ReleaseAll(Playback, n);
			SetOutput(1, n, 1);
			if (!ValueAsBoolean(m_Loop))
			{
				m_Playing = false;
				continue;
			}
			m_Phase %= length;
			m_Next = 0;
		}
		while (!m_Recording && m_Next < m_Clip.Count() && m_Clip.FrameAt(m_Next) <= m_Phase)
			Emit(Playback, n, m_Clip.At(m_Next++));
		m_Phase += m_Clock.Advance(advance);
	}

	// Match the event port's overflow contract. A partial group of pedal and
	// release messages could otherwise leave notes sounding indefinitely.
	if (m_Overflow)
	{
		m_Queued = 0;
		Event reset;
		reset.Type = Event::Reset;
		Queue(0, reset);
		m_Held[Live].clear();
		m_Held[Playback].clear();
		m_Tails[Live] = m_Tails[Playback] = 0;
		std::memset(m_Pedal, 0, sizeof(m_Pedal));
		m_Overflow = false;
	}

	if (out)
	{
		out->Clear(frames);
		const unsigned count = frames ? std::min<unsigned>(m_Queued, EventBuffer::Capacity) : 0;
		for (unsigned i = 0; i < count; ++i)
		{
			const Outgoing &event = m_OutputQueue[m_QueueHead];
			const unsigned offset = event.Frame > m_BlockFrame ? unsigned(event.Frame - m_BlockFrame) : 0;
			out->Append(offset, event.Value);
			m_QueueHead = (m_QueueHead + 1) % QUEUED_EVENTS;
		}
		m_Queued -= count;
	}
	else
		m_Queued = 0;

	m_BlockFrame += frames;
	ValueSetBoolean(m_PlayingState, m_Playing);
	ValueSetBoolean(m_RecordingState, m_Recording);
	SetFloat(m_Position, (float)((double)m_Phase / m_Clip.Rate()));
}

void PianoRollPlugin::ExecuteCommands()
{
	if (!m_AudioCH->IsCommandWaiting())
		return;

	const uint64_t start = Frames(m_Args.Start);
	const uint64_t end = Frames(m_Args.End);
	bool edited = false, moved = false;

	switch (m_AudioCH->GetCommand())
	{
		case PLAY:
			Start(false);

			break;
		case STOP:
			if (m_Playing)
				Stop(0);

			break;
		case RECORD:
			Start(true);

			break;
		case CLEAR:
			ReleaseAll(Playback, 0);
			m_Playing = m_Recording = false;
			m_RecordStatus[0] = 0;
			m_Clip.Clear(m_Rate);
			m_OriginalTempo = 0;
			m_Clock.Reset();
			m_Phase = 0;
			edited = true;

			break;
		case ADD_NOTE:
			edited = m_Clip.AddNote(start, end, 0, (unsigned)m_Args.Key, m_Args.Velocity);

			break;
		case EDIT_NOTE:
			edited = m_Args.Index >= 0 && m_Clip.EditNote((unsigned)m_Args.Index, start, end, (unsigned)m_Args.Key,
					m_Args.Velocity);

			break;
		case DELETE_NOTE:
			edited = m_Args.Index >= 0 && m_Clip.EditNote((unsigned)m_Args.Index, 0, 0, 0, 0, true);

			break;
		case SET_POSITION:
			// The position marker moved by hand, no further than the roll
			// goes: where a take starts, and where play goes on from once
			// what was sounding is let go. A take is not moved.
			if (!m_Recording && m_Args.Start >= 0)
			{
				ReleaseAll(Playback, 0);
				m_Clock.Reset();
				m_Phase = std::min(start, ClipLength());
				moved = true;
			}

			break;
	}

	if (!edited && !moved)
		return;

	m_Next = 0;

	while (m_Next < m_Clip.Count() && m_Clip.FrameAt(m_Next) < m_Phase)
		++m_Next;

	if (edited)
		Published();
}

void PianoRollPlugin::Describe(Description &d)
{
	d.Value(DeviceVersion);
	d.Separator(" ");
	m_Saved.Describe(d);
	d.Separator(" ");
	m_Loop.Describe(d);
	d.Separator(" ");
	m_Thru.Describe(d);
	d.Separator(" ");
	m_Length.Describe(d);
	if (m_OriginalTempo)
		d.Separator(" ").Value(m_OriginalTempo).Key("OriginalTempo", "FloatProperty");
}

void PianoRollPlugin::Apply(Description::Reader &r)
{
	int version;

	if (!r.More())
		return;

	// Named packages omit a zero device version.
	if (!r.Named())
		r.Value(version);
	m_Saved.Restore(r);
	m_Loop.Restore(r);
	m_Thru.Restore(r);
	m_Length.Restore(r);
	m_OriginalTempo = 0;
	if (r.Named())
	{
		const int index = r.Find("OriginalTempo");
		if (index >= 0)
			r.At(index, m_OriginalTempo);
	}
	else if (r.More())
		r.Value(m_OriginalTempo);
	if (!ClipTempo::Valid(m_OriginalTempo))
		m_OriginalTempo = 0;

	if (!m_Saved.Value.empty() && !m_Clip.Load(m_Saved.Value))
		m_Clip.Clear(m_Rate);

	m_Next = 0;
	Published();
}
