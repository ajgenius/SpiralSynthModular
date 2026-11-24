// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef PIANO_ROLL_PLUGIN_H
#define PIANO_ROLL_PLUGIN_H

#include "Device.h"
#include "VoicePool.h"
#include "ClipTempo.h"
#include "Event.h"
#include "EventClip.h"
#include <stdint.h>
#include <vector>

class PianoRollPlugin: public Spiral::Core::Device, public VoiceStateKey
{
public:
	PianoRollPlugin();
	virtual ~PianoRollPlugin();

	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void ExecuteCommands();
	virtual void Describe(Spiral::Core::Description &d);
	virtual void DescribeVoiceKey(Spiral::Core::Description &d) const;
	virtual bool UpdateProperty(std::string name);
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
		MAX_HELD = 1024
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
		int InputID;
		bool KeyDown;
		Spiral::Core::Event Off;
	};

	// A shared output can briefly receive a full input block and a transport
	// cleanup together. Carry the excess to the next writable block.
	enum { MAX_PEDAL_TAILS = 256, QUEUED_EVENTS = MAX_HELD * 6 };

	struct Outgoing
	{
		uint64_t Frame;
		Spiral::Core::Event Value;
	};

	struct Args
	{
		int Index, Key;
		float Velocity;
		double Start, End;
	};

	struct SavedSequence
	{
		std::string Clip;
		uint64_t ManualEnd;
		bool Automatic;
		double Tempo;
	};
	std::vector<SavedSequence> m_Sequences;
	std::string m_SequenceMode;
	uint64_t m_ContentEnd;
	float m_ImportedLength;
	bool RestoreSequences(const std::string &text);
	std::string SaveSequences() const;
	uint64_t ClipLength() const;
	void Start(bool record);
	void Stop(unsigned frame);
	void Queue(unsigned frame, const Spiral::Core::Event &value);
	void Emit(Source source, unsigned frame, const Spiral::Core::Event &event);
	int NextID();
	void ReleaseAll(Source source, unsigned frame);
	void QueuePedal(unsigned frame, int channel, float value);
	void QueueRelease(unsigned frame, const Spiral::Core::Event &off);
	void ReleaseTail(Source source, unsigned frame);
	void ReleaseNotes(Source source, unsigned frame, int channel, bool keysToo);
	void Pedal(Source source, unsigned frame, const Spiral::Core::Event &event);
	float OutputPedal(int channel) const;
	void InputEvent(const Spiral::Core::Event &event, unsigned frame, bool record);
	bool MergeRecording();
	void Published();
	void DescribeSettings(Spiral::Core::Description &d) const;
	uint64_t Frames(double seconds) const;

	Spiral::Core::EventClip m_Clip, m_Take;
	Spiral::Core::StringProperty m_Saved;
	uint64_t m_ClipRevision;
	char m_RecordStatus[80];
	Spiral::Core::ToggleProperty m_Loop, m_Thru, m_PlayingState, m_RecordingState;
	Spiral::Core::FloatProperty m_Length, m_Position;
	Spiral::Core::UnsignedProperty m_NoteCount;
	Args m_Args;
	std::vector<Held> m_Held[2];
	unsigned m_Tails[2];
	float m_Pedal[2][16];
	bool m_LiveConnected;
	Outgoing *m_OutputQueue;
	unsigned m_Queued, m_QueueHead;
	uint64_t m_BlockFrame;
	bool m_Overflow;
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
