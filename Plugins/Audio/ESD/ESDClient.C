// SPDX-License-Identifier: GPL-2.0-or-later
#include "ESDClient.h"
#include <esd.h>
#include <poll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <algorithm>
using namespace spiralcore;

ESDClient::ESDClient():
    m_ReadFD(-1),
    m_WriteFD(-1),
    m_Next(0),
    m_InputBytes(0),
    m_OutputBytes(0),
    m_OutputOffset(0)
{
}

ESDClient::~ESDClient()
{
	Detach();
}

bool ESDClient::Attach(const std::string &device, const AudioClientOptions &options)
{
	Detach();
	if (!options.BufferSize || options.BufferSize > 16384 || options.Samplerate < 8000 ||
		options.Samplerate > 192000 || options.InChannels > 2 || options.OutChannels > 2 ||
		(!options.InChannels && !options.OutChannels))
		return false;

	m_Options = options;
	const char *server = device.empty() || device == "default" ? NULL : device.c_str();
	if (options.OutChannels)
		m_WriteFD = esd_play_stream(static_cast<esd_format_t>(ESD_BITS16 | ESD_STREAM | ESD_PLAY |
			(options.OutChannels == 2 ? ESD_STEREO : ESD_MONO)), options.Samplerate, server, "SpiralSynthModular");
	if (options.InChannels)
		m_ReadFD = esd_record_stream(static_cast<esd_format_t>(ESD_BITS16 | ESD_STREAM | ESD_RECORD |
			(options.InChannels == 2 ? ESD_STEREO : ESD_MONO)), options.Samplerate, server, "SpiralSynthModular");

	if ((options.OutChannels && m_WriteFD < 0) || (options.InChannels && m_ReadFD < 0))
	{
		Detach();
		return false;
	}
	int handles[] = {m_ReadFD, m_WriteFD};
	for (unsigned n = 0; n < 2; ++n)
		if (handles[n] >= 0 && fcntl(handles[n], F_SETFL, O_NONBLOCK) < 0)
		{
			Detach();
			return false;
		}

#ifdef SO_NOSIGPIPE
	if (m_WriteFD >= 0)
	{
		int enabled = 1;
		if (setsockopt(m_WriteFD, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) < 0)
		{
			Detach();
			return false;
		}
	}
#endif

	m_Input.assign(options.BufferSize * options.InChannels, 0);
	m_Output.assign(options.BufferSize * options.OutChannels, 0);
	m_InputBytes = m_OutputBytes = m_OutputOffset = 0;
	m_Next = AudioMonotonicTime();
	m_Timing = AudioCycleTiming();
	return true;
}

void ESDClient::Detach()
{
	if (m_ReadFD >= 0)
		close(m_ReadFD);
	if (m_WriteFD >= 0)
		close(m_WriteFD);
	m_ReadFD = m_WriteFD = -1;
	m_Timing.Valid = false;
}

bool ESDClient::DrainOutput()
{
	if (m_OutputOffset == m_OutputBytes)
		return true;
	const char *bytes = reinterpret_cast<const char *>(&m_Output[0]);
	const ssize_t sent = send(m_WriteFD, bytes + m_OutputOffset, m_OutputBytes - m_OutputOffset,
#ifdef MSG_NOSIGNAL
				  MSG_NOSIGNAL
#else
				  0
#endif
	);
	if (sent > 0)
		m_OutputOffset += sent;
	return sent > 0 || (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR));
}

int ESDClient::WaitForCycle(unsigned milliseconds)
{
	if (!IsAttached())
		return -1;
	pollfd descriptors[2] = {{m_ReadFD, POLLIN, 0}, {m_WriteFD, POLLOUT, 0}};
	const int ready = poll(descriptors, 2, milliseconds);
	if (ready < 0)
		return errno == EINTR ? 0 : -1;
	if ((descriptors[0].revents | descriptors[1].revents) & (POLLERR | POLLHUP | POLLNVAL))
		return -1;
	if (m_WriteFD >= 0 && !DrainOutput())
		return -1;
	if (m_ReadFD >= 0 && m_InputBytes < m_Input.size() * sizeof(short))
	{
		char *bytes = reinterpret_cast<char *>(&m_Input[0]);
		const ssize_t got = read(m_ReadFD, bytes + m_InputBytes, m_Input.size() * sizeof(short) - m_InputBytes);
		if (got > 0)
			m_InputBytes += got;
		else if (!got || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR))
			return -1;
	}
	const double now = AudioMonotonicTime();
	if (now < m_Next || m_OutputOffset != m_OutputBytes || m_InputBytes != m_Input.size() * sizeof(short))
		return 0;

	const double period = double(m_Options.BufferSize) / m_Options.Samplerate;
	m_Timing.Frame += m_Options.BufferSize;
	m_Timing.CallbackTime = m_Timing.OutputTime = now;
	m_Timing.InputTime = now - period;
	m_Timing.Step = 1.0 / m_Options.Samplerate;
	m_Timing.Frames = m_Options.BufferSize;
	m_Timing.SampleRate = m_Options.Samplerate;
	m_Timing.Valid = m_Timing.Estimated = true;
	m_Next = std::max(m_Next + period, now);
	return 1;
}

bool ESDClient::Read(float *samples, unsigned frames)
{
	if (!samples || frames != m_Options.BufferSize || m_Input.empty() || m_InputBytes != m_Input.size() * sizeof(short))
		return false;
	for (size_t n = 0; n < m_Input.size(); ++n)
		samples[n] = m_Input[n] / 32768.0f;
	m_InputBytes = 0;
	return true;
}

bool ESDClient::Write(const float *samples, unsigned frames)
{
	if (!samples || frames != m_Options.BufferSize || m_Output.empty() || m_OutputOffset != m_OutputBytes)
		return false;
	for (size_t n = 0; n < m_Output.size(); ++n)
	{
		const float value = samples[n] == samples[n] ? samples[n] : 0;
		m_Output[n] = static_cast<short>(std::max(-1.0f, std::min(1.0f, value)) * 32767);
	}
	m_OutputOffset = 0;
	m_OutputBytes = m_Output.size() * sizeof(short);
	return DrainOutput();
}
