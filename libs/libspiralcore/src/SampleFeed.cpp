// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "SampleFeed.h"
#include <cstring>

namespace Spiral
{
namespace Core
{
	void SampleFeed::Publish(const float *samples, unsigned count)
	{
		Block &block = m_Blocks.writer_slot();
		if (count > MaxSamples)
			count = MaxSamples;
		if (samples && count)
			std::memcpy(block.Samples, samples, count * sizeof(float));
		else
			count = 0;
		block.Count = count;
		m_Blocks.publish();
	}

	bool SampleFeed::Take(std::vector<float> &out)
	{
		if (!m_Blocks.consume())
			return false;
		const Block &block = m_Blocks.reader_slot();
		out.assign(block.Samples, block.Samples + block.Count);
		return true;
	}
}
}
