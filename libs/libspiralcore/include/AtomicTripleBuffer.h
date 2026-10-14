/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
// Copyright (C) 2026 Andrew Johnson <acjgenius@earthlink.net>
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __SPICY_ATOMIC_TRIPLE_BUFFER_H__
#define __SPICY_ATOMIC_TRIPLE_BUFFER_H__

namespace Spicy
{
	template <typename T>
	class AtomicTripleBuffer
	{
		T m_Storage[3];
		unsigned m_Write;
		volatile unsigned m_Middle;
		unsigned m_Read;
		static const unsigned NEW_FLAG = 4;

		AtomicTripleBuffer(const AtomicTripleBuffer &);
		AtomicTripleBuffer &operator=(const AtomicTripleBuffer &);

		unsigned Exchange(unsigned value)
		{
			unsigned old;
			do
			{
				old = m_Middle;
			} while (!__sync_bool_compare_and_swap(&m_Middle, old, value));
			return old;
		}

		unsigned LoadAcquire() const
		{
			const unsigned value = m_Middle;
			__sync_synchronize();
			return value;
		}

	public:
		AtomicTripleBuffer(): m_Storage(), m_Write(0), m_Middle(2), m_Read(1)
		{
		}

		T &writer_slot() { return m_Storage[m_Write]; }
		const T &writer_slot() const { return m_Storage[m_Write]; }

		void publish()
		{
			const unsigned old = Exchange(m_Write | NEW_FLAG);
			m_Write = old & ~NEW_FLAG;
		}

		bool consume()
		{
			if ((LoadAcquire() & NEW_FLAG) == 0)
				return false;
			const unsigned old = Exchange(m_Read);
			m_Read = old & ~NEW_FLAG;
			return true;
		}

		T &reader_slot() { return m_Storage[m_Read]; }
		const T &reader_slot() const { return m_Storage[m_Read]; }

		bool has_new() const
		{
			return (LoadAcquire() & NEW_FLAG) != 0;
		}
	};
}

#endif
