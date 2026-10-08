// SPDX-License-Identifier: GPL-2.0-or-later
#include "PresentationClock.h"
#include <algorithm>
#include <cmath>
using namespace spiralcore;

PresentationClock::PresentationClock() : m_Ready(false), m_Native(false), m_Generation(1), m_MasterEpoch(0),
	m_Frame(0), m_NextTime(0), m_TargetFrame(0), m_MasterRate(0), m_Rate(0),
	m_Ahead(0), m_Wait(0), m_Step(0)
{
}

void PresentationClock::Reset()
{
	m_Ready = false;
}

bool PresentationClock::Prepare(double now, unsigned frames, double rate,
	const AudioCycleTiming *master, double ahead, AudioStamp &stamp)
{
	if (!frames || rate < 8000 || rate > 384000) return false;

	const bool native = master && master->Valid && master->Step > 0 &&
		master->SampleRate > 0 && now - master->CallbackTime < 0.25;
	const double nativeRate = native ? master->SampleRate : rate;
	const double step = native ? master->Step * nativeRate / rate : 1 / rate;
	// Changes of clock, format or required lookahead establish a fresh epoch.
	// Old queued audio is never replayed while new clients join the timeline.
	if ((native && master->Epoch != m_MasterEpoch) || m_Native != native || m_Rate != rate || m_MasterRate != nativeRate ||
		ahead > m_Ahead + 0.5 / rate || (m_Ready && now > m_NextTime + 0.05)) m_Ready = false;

	if (!m_Ready)
	{
		++m_Generation;
		m_Native = native;
		m_MasterEpoch = native ? master->Epoch : 0;
		m_Rate = rate;
		m_MasterRate = nativeRate;
		m_Ahead = ahead;
		m_NextTime = now + ahead;
		if (native)
		{
			// Start on the native sample grid, including its current DAC latency.
			const double offset = std::ceil((m_NextTime - master->OutputTime) / master->Step);
			m_TargetFrame = double(master->Frame) + offset;
			m_NextTime = master->OutputTime + offset * master->Step;
		}

		m_Ready = true;
	}

	m_Wait = std::max(0.0, m_NextTime - m_Ahead - now);
	if (m_Wait > 0.00001) return false;

	m_Step = step;
	if (native)
	{
		const double target = master->OutputTime + (m_TargetFrame - double(master->Frame)) * master->Step;
		const double error = target - m_NextTime;
		if (std::fabs(error) > std::max(0.01, 2 * frames / rate))
		{
			Reset();
			return Prepare(now, frames, rate, master, ahead, stamp);
		}

		// Bounded phase recovery preserves continuity at block boundaries. A
		// master timestamp correction must not create overlaps or FIFO backlog.
		const double correction = std::max(-step * 0.0005,
			std::min(step * 0.0005, error / (rate * 0.25)));
		m_Step += correction;
	}

	stamp.Frame = m_Frame;
	stamp.Generation = m_Generation;
	stamp.Time = m_NextTime;
	stamp.Step = m_Step;
	return true;
}

void PresentationClock::Commit(unsigned frames)
{
	m_Frame += frames;
	m_NextTime += frames * m_Step;
	m_TargetFrame += frames * m_MasterRate / m_Rate;
}
