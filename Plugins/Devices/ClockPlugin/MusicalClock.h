// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SSM_MUSICAL_CLOCK_H
#define SSM_MUSICAL_CLOCK_H

#include <algorithm>
#include <cmath>
#include <stdint.h>

class MusicalClock
{
public:
	MusicalClock(): m_Anchor(0), m_NextFrame(0), m_QuarterBPM(120), m_Quarters(0), m_QuarterRate(0), m_BeatSteps(0), m_NoteSteps(0), m_Sequence(1), m_ResetHigh(false), m_HaveFrame(false) {}

	void Reset()
	{
		m_Anchor = 0;
		m_HaveFrame = false;
		m_Quarters = m_QuarterRate = 0;
		m_Beat = Division();
		m_Note = Division();
		m_ResetHigh = false;
	}

	void Configure(uint64_t frame, unsigned rate, double bpm, unsigned tempoNote,
		unsigned beats, unsigned beatUnit, unsigned steps, unsigned bars, unsigned stepNote)
	{
		if (m_HaveFrame && frame != m_NextFrame)
		{
			m_Beat.known = m_Note.known = false;
			m_HaveFrame = false;
		}
		m_QuarterBPM = bpm * NoteQuarters(tempoNote);
		const double quarterRate = rate ? m_QuarterBPM / (60.0 * rate) : 0;
		if (quarterRate != m_QuarterRate)
		{
			if (m_QuarterRate)
			{
				m_Quarters = Position(frame);
				m_Anchor = frame;
			}
			m_QuarterRate = quarterRate;
		}

		const double beatSteps = std::max(1u, beatUnit) * std::max(1u, steps) / 4.0;
		const double noteSteps = 1.0 / NoteQuarters(stepNote);
		// Re-grid at the current musical position. A control edit between
		// divisions must not manufacture an edge on that sample.
		if (beatSteps != m_BeatSteps)
			m_Beat.known = false;
		if (noteSteps != m_NoteSteps)
			m_Note.known = false;
		m_BeatSteps = beatSteps;
		m_NoteSteps = noteSteps;
		m_Sequence = std::max(1u, beats) * std::max(1u, bars) * std::max(1u, steps);
	}

	bool Tick(uint64_t frame, bool rolling, bool reset)
	{
		if (reset && !m_ResetHigh)
		{
			m_Anchor = frame;
			m_Quarters = 0;
			m_Beat.known = m_Note.known = false;
		}
		m_ResetHigh = reset;
		if (!rolling)
			return false;

		m_HaveFrame = true;
		m_NextFrame = frame + 1;
		const double quarters = Position(frame);
		m_Note.Advance(quarters * m_NoteSteps);
		return m_Beat.Advance(quarters * m_BeatSteps) && m_Beat.index % m_Sequence == 0;
	}

	double QuarterBPM() const { return m_QuarterBPM; }
	bool BeatLevel() const { return m_Beat.level; }
	bool NoteLevel() const { return m_Note.level; }

private:
	// Stored indices are shared with the two note choices on the panel.
	static double NoteQuarters(unsigned note)
	{
		static const double values[] = {1, 1.5, 0.5, 0.25, 2, 3};
		return note < 6 ? values[note] : 1;
	}

	struct Division
	{
		Division(): index(0), known(false), level(false) {}

		bool Advance(double position)
		{
			const double rounded = position + 1e-9;
			const int64_t step = static_cast<int64_t>(std::floor(rounded));
			const bool edge = known ? index != step : rounded - step < 2e-9;
			known = true;
			index = step;
			if (edge)
				level = !level;
			return edge;
		}

		int64_t index;
		bool known, level;
	};

	double Position(uint64_t frame) const
	{
		const double elapsed = frame >= m_Anchor ? double(frame - m_Anchor) : -double(m_Anchor - frame);
		return m_Quarters + elapsed * m_QuarterRate;
	}

	uint64_t m_Anchor, m_NextFrame;
	double m_QuarterBPM, m_Quarters, m_QuarterRate, m_BeatSteps, m_NoteSteps;
	int64_t m_Sequence;
	bool m_ResetHigh, m_HaveFrame;
	Division m_Beat, m_Note;
};
#endif
