// SPDX-License-Identifier: GPL-2.0-or-later
#include "PianoRollPlugin.h"
#include "EventBuffer.h"
#include "SpiralIcon.xpm"
#include <algorithm>
#include <climits>
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
	m_NoteCount(Property::ReadOnly, 0, 0, MAX_NOTES, 1, 1),
	m_HeldCount(0),
	m_Recorded(new Recorded[MAX_RECORDED]),
	m_Scratch(new Outgoing[EventBuffer::Capacity]),
	m_ScratchCount(0),
	m_RecordedCount(0),
	m_Playing(false),
	m_Recording(false),
	m_LaunchHigh(false),
	m_StopHigh(false),
	m_Phase(0),
	m_RecordStart(0),
	m_Next(0),
	m_NextID(0),
	m_Rate(48000)
{
	std::memset(&m_Args, 0, sizeof(m_Args));
	std::memset(m_Rows, 0, sizeof(m_Rows));
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
	m_AudioCH->RegisterData("NoteRows", ChannelHandler::OUTPUT, m_Rows, sizeof(m_Rows), 'd');
	std::vector<std::string> none, note, edit, index;
	note.push_back("Start");
	note.push_back("End");
	note.push_back("Key");
	note.push_back("Velocity");
	edit.push_back("Index");
	edit.insert(edit.end(), note.begin(), note.end());
	index.push_back("Index");
	m_AudioCH->RegisterCommand("PLAY", PLAY, none);
	m_AudioCH->RegisterCommand("STOP", STOP, none);
	m_AudioCH->RegisterCommand("RECORD", RECORD, none);
	m_AudioCH->RegisterCommand("CLEAR", CLEAR, none);
	m_AudioCH->RegisterCommand("ADD_NOTE", ADD_NOTE, note);
	m_AudioCH->RegisterCommand("EDIT_NOTE", EDIT_NOTE, edit);
	m_AudioCH->RegisterCommand("DELETE_NOTE", DELETE_NOTE, index);
	m_Version = 0;
	SetUnsigned(DeviceVersion, 0);
	m_PluginInfo.Name = "PianoRoll";
	m_PluginInfo.Width = 420;
	m_PluginInfo.Height = 260;
	m_PluginInfo.NumInputs = 3;
	m_PluginInfo.NumOutputs = 2;
	m_PluginInfo.PortTips.push_back("Events");
	m_PluginInfo.PortTips.push_back("Launch CV");
	m_PluginInfo.PortTips.push_back("Stop CV");
	m_PluginInfo.PortTips.push_back("Events");
	m_PluginInfo.PortTips.push_back("End CV");
	m_Clip.Clear(m_Rate);
}

PianoRollPlugin::~PianoRollPlugin()
{
	delete[] m_Recorded;
	delete[] m_Scratch;
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
	return seconds > 0 ? (uint64_t)(seconds * m_Rate + 0.5) : 0;
}

uint64_t PianoRollPlugin::ClipLength() const
{
	const uint64_t set = Frames(AsFloat(m_Length));

	return set ? set : m_Clip.Length();
}

void PianoRollPlugin::Emit(EventBuffer *, unsigned frame, const Event &event)
{
	if (m_ScratchCount == EventBuffer::Capacity)
		return;

	Event value = event;

	if (event.Type == Event::NoteOn)
	{
		if (m_HeldCount == MAX_HELD || m_NextID == INT_MAX)
			return;

		Held &held = m_Held[m_HeldCount++];
		held.Channel = event.Channel;
		held.ClipID = event.ID;
		held.OutputID = ++m_NextID;
		held.NoteNum = event.NoteNum;
		held.Frequency = event.Frequency;
		value.ID = held.OutputID;
	}
	else if (event.Type == Event::NoteOff)
	{
		unsigned i = 0;

		while (i < m_HeldCount && !(m_Held[i].Channel == event.Channel && m_Held[i].ClipID == event.ID))
			++i;

		if (i == m_HeldCount)
			return;

		value.ID = m_Held[i].OutputID;
		m_Held[i] = m_Held[--m_HeldCount];
	}
	else if (event.Type == Event::Reset)
		return;

	Outgoing &out = m_Scratch[m_ScratchCount];
	out.Frame = frame;
	out.Order = m_ScratchCount++;
	out.Value = value;
}

void PianoRollPlugin::ReleaseAll(EventBuffer *, unsigned frame)
{
	for (unsigned i = 0; i < m_HeldCount && m_ScratchCount < EventBuffer::Capacity; ++i)
	{
		Event off;
		off.Type = Event::NoteOff;
		off.ID = m_Held[i].OutputID;
		off.Channel = m_Held[i].Channel;
		off.NoteNum = m_Held[i].NoteNum;
		off.Frequency = m_Held[i].Frequency;
		Outgoing &out = m_Scratch[m_ScratchCount];
		out.Frame = frame;
		out.Order = m_ScratchCount++;
		out.Value = off;
	}

	m_HeldCount = 0;
}

void PianoRollPlugin::Start(bool record)
{
	// Play starts from the top. Record starts where the roll is: where it
	// last stopped, or where it has got to if it is playing.
	if (!m_Playing && !record)
	{
		m_Phase = 0;
		m_Next = 0;
	}

	m_Playing = true;

	if (record && !m_Recording)
	{
		// The roll under a take is not heard; what was sounding stops.
		ReleaseAll(NULL, 0);
		m_Recording = true;
		m_RecordStart = m_Phase;
		m_RecordedCount = 0;
	}
}

void PianoRollPlugin::Stop(EventBuffer *out, unsigned frame)
{
	ReleaseAll(out, frame);

	if (m_Recording)
		MergeRecording();

	m_Playing = false;
	m_Recording = false;
}

void PianoRollPlugin::PlayWindow(EventBuffer *out, uint64_t from, uint64_t to, unsigned offset)
{
	while (m_Next < m_Clip.Count() && m_Clip.FrameAt(m_Next) < to)
	{
		const uint64_t frame = m_Clip.FrameAt(m_Next);

		if (frame >= from)
			Emit(out, offset + (unsigned)(frame - from), m_Clip.At(m_Next));

		++m_Next;
	}
}

void PianoRollPlugin::Record(const EventBuffer *in, EventBuffer *)
{
	for (unsigned i = 0; in && i < in->Count(); ++i)
	{
		const Event &event = in->At(i);

		// Whatever comes in is recorded, on any channel; a reset is not,
		// as none is played back.
		if (m_Recording && m_RecordedCount < MAX_RECORDED && event.Type != Event::Reset)
		{
			m_Recorded[m_RecordedCount].Frame = m_Phase + in->Frame(i);
			m_Recorded[m_RecordedCount++].Event = event;
		}

		if (ValueAsBoolean(m_Thru) && m_ScratchCount < EventBuffer::Capacity)
		{
			Outgoing &out = m_Scratch[m_ScratchCount];
			out.Frame = in->Frame(i);
			out.Order = m_ScratchCount++;
			out.Value = event;
		}
	}
}

// The take replaces what it ran over, from where Record began to where it
// stopped: the roll before it stays, with notes sounding into it ended there,
// and the roll after it stays, with notes it stopped in the middle of starting
// again there. A take that never ran leaves the roll as it was.
void PianoRollPlugin::MergeRecording()
{
	if (m_Phase == m_RecordStart)
	{
		m_RecordedCount = 0;

		return;
	}

	int base = 0;

	for (unsigned i = 0; i < m_Clip.Count(); ++i)
		base = std::max(base, m_Clip.At(i).ID);

	EventClip merged;

	if (!merged.CopyBefore(m_Clip, m_RecordStart))
	{
		m_RecordedCount = 0;

		return;
	}

	for (unsigned i = 0; i < m_RecordedCount; ++i)
	{
		Event &event = m_Recorded[i].Event;

		// Only a note has an ID to keep clear of the clip's.
		if (event.Type == Event::NoteOn || event.Type == Event::NoteOff)
			event.ID = base + 1 + event.ID;

		merged.Append(m_Recorded[i].Frame, event);
	}

	// A roll too full to take it all keeps what fitted.
	merged.AppendFrom(m_Clip, m_Phase);
	m_Clip.Swap(merged);
	m_RecordedCount = 0;
	Published();
}

void PianoRollPlugin::Published()
{
	std::vector<NoteSpan> notes;
	m_Clip.Notes(notes);
	unsigned count = 0;

	for (size_t i = 0; i < notes.size() && count < MAX_NOTES; ++i)
	{
		double *row = m_Rows[count++];
		row[NOTE_START] = (double)notes[i].Start / m_Clip.Rate();
		row[NOTE_END] = (double)notes[i].End / m_Clip.Rate();
		row[NOTE_KEY] = notes[i].Note.NoteNum;
		row[NOTE_VELOCITY] = notes[i].Note.Volume;
		row[NOTE_INDEX] = (double)notes[i].OnEvent;
		row[NOTE_CHANNEL] = notes[i].Note.Channel;
	}

	// Only the notes there are go to the panel.
	m_AudioCH->SetUsed("NoteRows", (int)(count * sizeof(m_Rows[0])));
	SetUnsigned(m_NoteCount, count);
	std::string text = m_Clip.Save();
	std::replace(text.begin(), text.end(), '\n', ' ');
	m_Saved.Value = text;
}

void PianoRollPlugin::Execute()
{
	EventBuffer *out = GetEventOutput(0);
	const EventBuffer *in = GetEventInput(0);
	const unsigned frames = (unsigned)m_HostInfo->BUFSIZE;

	for (unsigned n = 0; n < frames; ++n)
	{
		SetOutput(1, n, 0);
		const bool launch = InputExists(1) && GetInput(1, n) > 0.1f;
		const bool stop = InputExists(2) && GetInput(2, n) > 0.1f;

		if (launch && !m_LaunchHigh)
			Start(false);

		if (stop && !m_StopHigh && m_Playing)
			Stop(out, n);

		m_LaunchHigh = launch;
		m_StopHigh = stop;
	}

	Record(in, out);

	if (m_Playing)
	{
		const uint64_t length = ClipLength();
		unsigned offset = 0;

		while (offset < frames)
		{
			if (length && m_Phase >= length && !m_Recording)
			{
				if (!ValueAsBoolean(m_Loop))
				{
					Stop(out, offset);
					SetOutput(1, offset, 1);

					break;
				}

				ReleaseAll(out, offset);
				m_Phase = 0;
				m_Next = 0;
			}

			const uint64_t left = length && !m_Recording && m_Phase < length ? length - m_Phase : frames - offset;
			const unsigned span = (unsigned)std::min<uint64_t>(left, frames - offset);

			// The roll under a take is being written over, not played.
			if (!m_Recording)
				PlayWindow(out, m_Phase, m_Phase + span, offset);
			m_Phase += span;
			offset += span;
		}
	}

	std::stable_sort(m_Scratch, m_Scratch + m_ScratchCount);

	if (out)
	{
		out->Clear(frames);

		for (unsigned i = 0; i < m_ScratchCount; ++i)
			out->Append(std::min(m_Scratch[i].Frame, frames - 1), m_Scratch[i].Value);
	}

	m_ScratchCount = 0;
	ValueSetBoolean(m_PlayingState, m_Playing);
	ValueSetBoolean(m_RecordingState, m_Recording);
	SetFloat(m_Position, (float)((double)m_Phase / m_Rate));
}

void PianoRollPlugin::ExecuteCommands()
{
	if (!m_AudioCH->IsCommandWaiting())
		return;

	const uint64_t start = Frames(m_Args.Start);
	const uint64_t end = Frames(m_Args.End);
	bool edited = false;

	switch (m_AudioCH->GetCommand())
	{
		case PLAY:
			Start(false);

			break;
		case STOP:
			if (m_Playing)
				Stop(NULL, 0);

			break;
		case RECORD:
			Start(true);

			break;
		case CLEAR:
			ReleaseAll(NULL, 0);
			m_Clip.Clear(m_Rate);
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
	}

	if (!edited)
		return;

	m_Next = 0;

	while (m_Next < m_Clip.Count() && m_Clip.FrameAt(m_Next) < m_Phase)
		++m_Next;

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
}

void PianoRollPlugin::Apply(Description::Reader &r)
{
	int version;

	if (!r.More())
		return;

	r.Value(version);
	m_Saved.Restore(r);
	m_Loop.Restore(r);
	m_Thru.Restore(r);
	m_Length.Restore(r);

	if (!m_Saved.Value.empty() && !m_Clip.Load(m_Saved.Value))
		m_Clip.Clear(m_Rate);

	m_Next = 0;
	Published();
}
