// SPDX-License-Identifier: GPL-2.0-or-later
#include "TimedAudioBuffer.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace spiralcore
{
namespace
{
	const unsigned Phases = 512;
	const double Pi = 3.14159265358979323846;
}

TimedAudioBuffer::TimedAudioBuffer() : m_Capacity(0), m_Mask(0), m_Channels(0),
	m_Taps(64), m_Read(0), m_Write(0), m_Search(0), m_Cutoff(1) {}

bool TimedAudioBuffer::Configure(unsigned capacity, unsigned channels, double inputRate, double outputRate)
{
	if (capacity < 128 || capacity > (1U << 20) || !channels || channels > 256 ||
		!(inputRate >= 8000 && inputRate <= 384000) || !(outputRate >= 8000 && outputRate <= 384000))
		return false;

	m_Taps = 64 * unsigned(std::ceil(std::max(1.0, inputRate / outputRate)));
	capacity = std::max(capacity, m_Taps * 4);
	m_Capacity = 1;
	while (m_Capacity < capacity) m_Capacity <<= 1;

	m_Mask = m_Capacity - 1;
	m_Channels = channels;
	m_Read = m_Write = m_Search = 0;
	m_Positions.resize(m_Capacity);
	m_Samples.assign(size_t(m_Capacity) * channels, 0);
	m_Filter.resize((Phases + 1) * m_Taps);
	m_Cutoff = outputRate < inputRate ? 0.94 * outputRate / inputRate : 1;

	// Symmetric windowed sinc: lookahead supplies the future half, so the
	// filter adds no unreported presentation delay. Normalize each phase.
	for (unsigned phase = 0; phase <= Phases; ++phase)
	{
		double sum = 0;
		for (unsigned tap = 0; tap < m_Taps; ++tap)
		{
			const double x = int(tap) - int(m_Taps / 2 - 1) - double(phase) / Phases;
			const double sinc = std::fabs(x) < 1e-12 ? m_Cutoff : std::sin(Pi * x * m_Cutoff) / (Pi * x);
			const double window = std::fabs(x) > m_Taps / 2 ? 0 : 0.42 + 0.5 * std::cos(Pi * x / (m_Taps / 2)) + 0.08 * std::cos(Pi * x / (m_Taps / 4));
			m_Filter[phase * m_Taps + tap] = float(sinc * window);
			sum += sinc * window;
		}

		for (unsigned tap = 0; tap < m_Taps; ++tap)
			m_Filter[phase * m_Taps + tap] /= float(sum);
	}

	return true;
}

unsigned TimedAudioBuffer::Buffered() const
{
	const unsigned write = __sync_fetch_and_add(&m_Write, 0);
	const unsigned read = __sync_fetch_and_add(&m_Read, 0);
	return std::min(write - read, m_Capacity);
}

bool TimedAudioBuffer::Write(const float *samples, unsigned frames, const AudioStamp &stamp)
{
	if (!m_Capacity || !samples || !frames || !(stamp.Step > 0 && stamp.Step < 1) ||
		!(stamp.Time >= 0 && stamp.Time < 1e12) || !stamp.Generation)
		return false;

	const unsigned write = m_Write;
	const unsigned read = __sync_fetch_and_add(&m_Read, 0);
	if (frames > m_Capacity - (write - read)) return false;

	for (unsigned n = 0; n < frames; ++n)
	{
		const unsigned slot = (write + n) & m_Mask;
		Position &position = m_Positions[slot];
		position.Time = stamp.Time + n * stamp.Step;
		position.Step = stamp.Step;
		position.Frame = stamp.Frame + n;
		position.Generation = stamp.Generation;
		memcpy(&m_Samples[size_t(slot) * m_Channels], samples + size_t(n) * m_Channels,
			m_Channels * sizeof(float));
	}

	__sync_synchronize();
	__sync_lock_test_and_set(&m_Write, write + frames);
	return true;
}

bool TimedAudioBuffer::Covered(unsigned index, unsigned read, unsigned write, unsigned generation) const
{
	return index - read < write - read && m_Positions[index & m_Mask].Generation == generation;
}

unsigned TimedAudioBuffer::Read(float *samples, unsigned frames, double time, double step, unsigned generation)
{
	if (!samples || !m_Channels) return 0;

	memset(samples, 0, size_t(frames) * m_Channels * sizeof(float));
	if (!(time >= 0 && time < 1e12) || !(step > 0 && step < 1)) return 0;

	unsigned read = m_Read;
	const unsigned write = __sync_fetch_and_add(&m_Write, 0);
	while (read != write && int32_t(m_Positions[read & m_Mask].Generation - generation) < 0) ++read;

	unsigned cursor = m_Search;
	if (cursor - read >= write - read) cursor = read;

	unsigned covered = 0;
	for (unsigned frame = 0; frame < frames && cursor != write; ++frame)
	{
		const double target = time + frame * step;
		while (Covered(cursor + 1, read, write, generation) &&
			m_Positions[(cursor + 1) & m_Mask].Time <= target + 1e-10)
			++cursor;

		const Position &center = m_Positions[cursor & m_Mask];
		const double fraction = (target - center.Time) / center.Step;
		if (center.Generation != generation || fraction < -1e-4 || fraction >= 1 - 1e-6) continue;

		float *output = samples + size_t(frame) * m_Channels;
		++covered;
		if (std::fabs(fraction) < 1e-4 && m_Cutoff == 1)
		{
			memcpy(output, &m_Samples[size_t(cursor & m_Mask) * m_Channels], m_Channels * sizeof(float));
			continue;
		}

		const double phase = std::max(0.0, std::min(double(Phases) - 1e-6, fraction * Phases));
		const unsigned lower = unsigned(phase);
		const float blend = float(phase - lower);
		for (unsigned tap = 0; tap < m_Taps; ++tap)
		{
			const unsigned index = cursor + tap - (m_Taps / 2 - 1);
			if (!Covered(index, read, write, generation)) continue;

			const Position &position = m_Positions[index & m_Mask];
			// A dropped block or reset is a hole, not a very slow sample.
			if (position.Frame + (m_Taps / 2 - 1) != center.Frame + tap ||
				std::fabs(position.Time - (center.Time + (int(tap) - int(m_Taps / 2 - 1)) * center.Step)) > center.Step * 0.25)
				continue;

			const float a = m_Filter[lower * m_Taps + tap];
			const float coefficient = a + blend * (m_Filter[(lower + 1) * m_Taps + tap] - a);
			const float *input = &m_Samples[size_t(index & m_Mask) * m_Channels];
			for (unsigned channel = 0; channel < m_Channels; ++channel)
				output[channel] += input[channel] * coefficient;
		}
	}

	// Retain filter history. Only this consumer advances the read cursor.
	m_Search = cursor;
	if (cursor - read > m_Taps) read = cursor - m_Taps;

	__sync_synchronize();
	__sync_lock_test_and_set(&m_Read, read);
	return covered;
}
}
