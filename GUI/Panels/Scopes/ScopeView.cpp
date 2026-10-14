// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralPlugin
 *  Copyleft (C) 2000 David Griffiths <dave@pawfal.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include "ScopeView.h"
#include <FL/fl_draw.H>

namespace SpiralPanels
{
	ScopeView::ScopeView(int x, int y, int w, int h):
		Fl_Widget(x, y, w, h),
		m_Attenuation(0),
		m_TimeBase(1)
	{
		color(fl_rgb_color(20, 60, 20));
	}

	void ScopeView::SetSamples(const std::vector<float> &samples)
	{
		m_Samples = samples;
		redraw();
	}

	void ScopeView::SetAttenuation(double attenuation)
	{
		m_Attenuation = attenuation;
		redraw();
	}

	void ScopeView::SetTimeBase(double timeBase)
	{
		m_TimeBase = timeBase > 0 ? timeBase : 0.001;
		redraw();
	}

	void ScopeView::draw()
	{
		const int middle = h() / 2;
		fl_color(color());
		fl_rectf(x(), y(), w(), h());
		fl_color(fl_rgb_color(155, 155, 50));
		fl_line(x(), y() + middle, x() + w(), y() + middle);

		fl_push_clip(x(), y(), w(), h());
		fl_color(fl_rgb_color(100, 200, 100));
		const double gain = (1.0 - m_Attenuation) * middle;
		float previous = 0;
		for (int n = 0; n < w(); ++n)
		{
			const size_t p = (size_t)(n * m_TimeBase);
			const float next = p < m_Samples.size() ? (float)(m_Samples[p] * gain) : 0.0f;
			if (n > 0)
				fl_line(x() + n - 1, (int)(y() + middle - previous), x() + n, (int)(y() + middle - next));
			previous = next;
		}
		fl_pop_clip();
	}
}
