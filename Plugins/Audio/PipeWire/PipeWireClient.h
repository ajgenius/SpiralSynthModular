// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRALCORE_PIPEWIRE_CLIENT_H
#define SPIRALCORE_PIPEWIRE_CLIENT_H
#include "AudioClient.h"
#include "TimedAudioBuffer.h"
#include <pipewire/pipewire.h>
#include <atomic>
#include <vector>

namespace spiralcore
{
	class PipeWireClient: public AudioClient
	{
	public:
		PipeWireClient();
		~PipeWireClient();
		bool Attach(const std::string &device, const AudioClientOptions &options);
		void Detach();

		bool IsAttached() const
		{
			return m_Attached.load();
		}

		bool IsCallbackDriven() const
		{
			return true;
		}

		void SetCallback(void (*run)(void *, unsigned), void *context);
		bool Start();

		unsigned long GetBufferSize() const
		{
			return m_Quantum.load();
		}

		unsigned long GetSampleRate() const
		{
			return m_Options.Samplerate;
		}

		bool GetCycleTiming(AudioCycleTiming &value) const
		{
			value = m_Timing;
			return value.Valid;
		}

		double GetInputLatency() const
		{
			return m_InputDelay.load() / 1000000.0 + (m_Options.OutChannels ?
		4.0 * m_Quantum.load() / m_Options.Samplerate : 0);
		}

		double GetOutputLatency() const
		{
			return m_OutputDelay.load() / 1000000.0;
		}

		bool Read(float *samples, unsigned frames);
		bool Write(const float *samples, unsigned frames);

	private:
		struct Stream
		{
			PipeWireClient *Owner;
			pw_stream *Handle;
			pw_stream_events Events;
			std::atomic<int> State;
			bool Input;
			uint64_t Frame;

			Stream():
			    Owner(NULL),
			    Handle(NULL),
			    State(PW_STREAM_STATE_UNCONNECTED),
			    Input(false),
			    Frame(0)
			{
			}
		};

		bool CreateStream(Stream &stream, bool input, const std::string &device);
		bool WaitReady(Stream &stream);
		static void StateChanged(void *, pw_stream_state, pw_stream_state, const char *);
		static void Process(void *);
		void Transfer(Stream &stream);
		bool Timestamp(Stream &stream, unsigned frames, AudioCycleTiming &timing);
		pw_thread_loop *m_Loop;
		bool m_LoopStarted;
		std::atomic<bool> m_Attached;
		std::atomic<unsigned> m_InputDelay, m_OutputDelay, m_Quantum;
		AudioClientOptions m_Options;
		Stream m_Input, m_Output;
		TimedAudioBuffer m_Capture;
		AudioCycleTiming m_Timing;
		float *m_CurrentInput, *m_CurrentOutput;
		unsigned m_CurrentFrames;
		void (*m_Run)(void *, unsigned);
		void *m_Context;
		PipeWireClient(const PipeWireClient &);
		PipeWireClient &operator=(const PipeWireClient &);
	};
}
#endif
