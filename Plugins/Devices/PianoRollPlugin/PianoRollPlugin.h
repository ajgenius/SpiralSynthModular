// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef PIANO_ROLL_PLUGIN_H
#define PIANO_ROLL_PLUGIN_H

#include "Device.h"
#include "ClipTempo.h"
#include "Event.h"
#include "EventClip.h"
#include <stdint.h>
#include <vector>

class PianoRollPlugin: public Spiral::Core::Device
{
public:
	PianoRollPlugin();
	virtual ~PianoRollPlugin();

	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void ExecuteCommands();
	virtual void Describe(Spiral::Core::Description &d);
	virtual void Apply(Spiral::Core::Description::Reader &r);

	enum Commands
	{
		NONE,
		PLAY,
		STOP,
		RECORD,
		CLEAR,
		ADD_NOTE,
		EDIT_NOTE,
		DELETE_NOTE,
		SET_POSITION
	};

	enum
	{
		MAX_HELD = 128
	};

	// A note is a row of the table handed to the panel: its start and end
	// in seconds, key, velocity, the event that names it back, its channel.
	enum
	{
		NOTE_START,
		NOTE_END,
		NOTE_KEY,
		NOTE_VELOCITY,
		NOTE_INDEX,
		NOTE_CHANNEL,
		NOTE_CELLS
	};

	const Spiral::Core::EventClip &Clip() const { return m_Clip; }
	bool Playing() const { return m_Playing; }
	bool Recording() const { return m_Recording; }

private:
	enum Source { Playback, Live };

	struct Held
	{
		int Channel, InputID, OutputID, NoteNum;
		float Frequency;
	};

	struct Outgoing
	{
		unsigned Frame, Order;
		Spiral::Core::Event Value;

		bool operator<(const Outgoing &other) const
		{
			return Frame != other.Frame ? Frame < other.Frame : Order < other.Order;
		}
	};

	struct Args
	{
		int Index, Key;
		float Velocity;
		double Start, End;
	};

	uint64_t ClipLength() const;
	void Start(bool record);
	void Stop(Spiral::Core::EventBuffer *out, unsigned frame);
	void Queue(unsigned frame, const Spiral::Core::Event &value);
	void Emit(Source source, unsigned frame, const Spiral::Core::Event &event);
	int NextID();
	void ReleaseAll(Source source, unsigned frame);
	void InputEvent(const Spiral::Core::Event &event, unsigned frame, bool record);
	bool MergeRecording();
	void Published();
	uint64_t Frames(double seconds) const;

	Spiral::Core::EventClip m_Clip, m_Take;
	Spiral::Core::StringProperty m_Saved;
	char m_RecordStatus[80];
	Spiral::Core::ToggleProperty m_Loop, m_Thru, m_PlayingState, m_RecordingState;
	Spiral::Core::FloatProperty m_Length, m_Position;
	Spiral::Core::UnsignedProperty m_NoteCount;
	Args m_Args;
	std::vector<Held> m_Held[2];
	Outgoing *m_Scratch;
	unsigned m_ScratchCount;
	bool m_Playing, m_Recording, m_LaunchHigh, m_StopHigh;
	// Where the roll is. Play starts again from the top; Record starts
	// here and overwrites as it goes, from m_RecordStart on.
	uint64_t m_Phase, m_RecordStart;
	ClipTempo m_Clock;
	double m_OriginalTempo;
	bool m_TempoPaused;
	unsigned m_Next;
	int m_NextID;
	bool m_IDWrapped;
	unsigned m_Rate;
};

#endif
