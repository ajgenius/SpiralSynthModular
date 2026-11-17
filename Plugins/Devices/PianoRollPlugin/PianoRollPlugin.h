// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef PIANO_ROLL_PLUGIN_H
#define PIANO_ROLL_PLUGIN_H

#include "Device.h"
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
		DELETE_NOTE
	};

	enum
	{
		// Every note a clip can hold is shown.
		MAX_NOTES = Spiral::Core::EventClip::MaxEvents / 2,
		MAX_HELD = 128,
		MAX_RECORDED = 4096
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
	struct Held
	{
		int Channel, ClipID, OutputID, NoteNum;
		float Frequency;
	};

	struct Recorded
	{
		uint64_t Frame;
		Spiral::Core::Event Event;
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
	void Emit(Spiral::Core::EventBuffer *out, unsigned frame, const Spiral::Core::Event &event);
	void ReleaseAll(Spiral::Core::EventBuffer *out, unsigned frame);
	void PlayWindow(Spiral::Core::EventBuffer *out, uint64_t from, uint64_t to, unsigned offset);
	void Record(const Spiral::Core::EventBuffer *in, Spiral::Core::EventBuffer *out);
	void MergeRecording();
	void Published();
	uint64_t Frames(double seconds) const;

	Spiral::Core::EventClip m_Clip;
	Spiral::Core::StringProperty m_Saved;
	Spiral::Core::ToggleProperty m_Loop, m_Thru, m_PlayingState, m_RecordingState;
	Spiral::Core::FloatProperty m_Length, m_Position;
	Spiral::Core::UnsignedProperty m_NoteCount;
	Args m_Args;
	double m_Rows[MAX_NOTES][NOTE_CELLS];
	Held m_Held[MAX_HELD];
	unsigned m_HeldCount;
	Recorded *m_Recorded;
	Outgoing *m_Scratch;
	unsigned m_ScratchCount;
	unsigned m_RecordedCount;
	bool m_Playing, m_Recording, m_LaunchHigh, m_StopHigh;
	// Where the roll is. Play starts again from the top; Record starts
	// here and overwrites as it goes, from m_RecordStart on.
	uint64_t m_Phase, m_RecordStart;
	unsigned m_Next;
	int m_NextID;
	unsigned m_Rate;
};

#endif
