// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRALCORE_ESD_CLIENT_H
#define SPIRALCORE_ESD_CLIENT_H
#include "AudioClient.h"
#include <vector>

namespace spiralcore
{
	// Compatibility backend: ESD exposes no ADC/DAC timestamps. Queue times are
	// explicitly estimated; use a native timestamp backend for measured alignment.
	class ESDClient: public AudioClient
	{
	public:
		ESDClient();
		~ESDClient();
		bool Attach(const std::string &device, const AudioClientOptions &options);
		void Detach();

		bool IsAttached() const
		{
			return m_ReadFD >= 0 || m_WriteFD >= 0;
		}

		unsigned long GetBufferSize() const
		{
			return m_Options.BufferSize;
		}

		unsigned long GetSampleRate() const
		{
			return m_Options.Samplerate;
		}

		int WaitForCycle(unsigned milliseconds);

		bool GetCycleTiming(AudioCycleTiming &value) const
		{
			value = m_Timing;
			return value.Valid;
		}

		bool Read(float *samples, unsigned frames);
		bool Write(const float *samples, unsigned frames);

	private:
		bool DrainOutput();
		int m_ReadFD, m_WriteFD;
		AudioClientOptions m_Options;
		AudioCycleTiming m_Timing;
		double m_Next;
		std::vector<short> m_Input, m_Output;
		size_t m_InputBytes, m_OutputBytes, m_OutputOffset;
		ESDClient(const ESDClient &);
		ESDClient &operator=(const ESDClient &);
	};
}
#endif
