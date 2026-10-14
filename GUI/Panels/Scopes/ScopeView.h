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
#ifndef SPIRAL_PANELS_SCOPE_VIEW_H
#define SPIRAL_PANELS_SCOPE_VIEW_H

#include <FL/Fl_Widget.H>
#include <vector>

namespace SpiralPanels
{
	class ScopeView: public Fl_Widget
	{
	public:
		ScopeView(int x, int y, int w, int h);

		void SetSamples(const std::vector<float> &samples);
		void SetAttenuation(double attenuation);
		double Attenuation() const { return m_Attenuation; }
		void SetTimeBase(double timeBase);
		double TimeBase() const { return m_TimeBase; }

	protected:
		virtual void draw();

		std::vector<float> m_Samples;
		double m_Attenuation;
		double m_TimeBase;
	};
}

#endif
