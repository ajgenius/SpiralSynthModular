// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef POSH_SAMPLER_VOICES_H
#define POSH_SAMPLER_VOICES_H

class SamplerVoices
{
public:
	enum { Slots = 8 };
	SamplerVoices(): m_Count(0), m_Width(1), m_Active(0), m_Occupied(0)
	{
		for (int s = 0; s < Slots; ++s)
			m_Lane[s] = -1;
	}

	int Lane(int slot) const { return m_Lane[slot]; }
	int Width() const { return m_Width; }

	// Keep existing lane numbers when the patch changes width.
	unsigned Resize(unsigned voices)
	{
		m_Width = voices == 0 ? 1 : voices > Slots ? Slots : voices;
		unsigned stopped = 0;
		for (int s = 0; s < Slots; ++s)
			if (m_Lane[s] >= m_Width)
			{
				stopped |= 1u << s;
				Release(s);
			}
		return stopped;
	}

	// Resolve all edges before writing audio for this frame. Ended samples
	// free their lanes first. A retrigger keeps its lane and renews its age.
	unsigned Route(unsigned playing, unsigned attacks)
	{
		if (playing == m_Active && !attacks)
			return 0;
		const unsigned starts = playing & ~m_Active;
		for (int s = 0; s < Slots; ++s)
		{
			if (m_Lane[s] < 0)
				continue;
			if (!(playing & (1u << s)))
				Release(s);
			else if (attacks & (1u << s))
			{
				const int lane = m_Lane[s];
				Release(s);
				Assign(s, lane);
			}
		}
		unsigned stolen = 0;
		for (int s = 0; s < Slots; ++s)
		{
			if (!(starts & (1u << s)))
				continue;
			int lane = 0;
			if (m_Count == m_Width)
			{
				const int oldest = m_Order[0];
				lane = m_Lane[oldest];
				stolen |= 1u << oldest;
				Release(oldest);
			}
			else
				while (m_Occupied & (1u << lane))
					++lane;
			Assign(s, lane);
		}
		return stolen;
	}

private:
	void Release(int slot)
	{
		m_Occupied &= ~(1u << m_Lane[slot]);
		m_Active &= ~(1u << slot);
		m_Lane[slot] = -1;
		int i = 0;
		while (m_Order[i] != slot)
			++i;
		for (++i; i < m_Count; ++i)
			m_Order[i - 1] = m_Order[i];
		--m_Count;
	}

	void Assign(int slot, int lane)
	{
		m_Lane[slot] = lane;
		m_Order[m_Count++] = slot;
		m_Occupied |= 1u << lane;
		m_Active |= 1u << slot;
	}

	int m_Lane[Slots], m_Order[Slots], m_Count, m_Width;
	unsigned m_Active, m_Occupied;
};

#endif
