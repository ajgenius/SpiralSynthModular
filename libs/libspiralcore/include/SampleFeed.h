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
#ifndef SPIRAL_SAMPLE_FEED_H
#define SPIRAL_SAMPLE_FEED_H

#include "AtomicTripleBuffer.h"
#include <vector>

namespace Spiral
{
namespace Core
{
	class Sample;

	// A monitor can publish a selected input without changing its signal output.
	class DisplayInputSource
	{
	public:
		virtual ~DisplayInputSource() {}
		virtual const Sample *DisplayInput(unsigned port) = 0;
	};

	class SampleFeed
	{
	public:
		static const unsigned MaxSamples = 8192;

		void Publish(const float *samples, unsigned count);
		bool Take(std::vector<float> &out);

	protected:
		struct Block
		{
			float Samples[MaxSamples];
			unsigned Count;

			Block(): Count(0) {}
		};

		Spicy::AtomicTripleBuffer<Block> m_Blocks;
	};
}
}

#endif
