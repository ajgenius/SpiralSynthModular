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
#ifndef SPIRAL_PANELS_PANELMETERBAR_H
#define SPIRAL_PANELS_PANELMETERBAR_H

#include "PanelControl.h"
#include <string>
#include <vector>

class Fl_VU_Meter;

namespace Spiral
{
namespace Core
{
	class SampleFeed;
}
}

namespace SpiralPanels
{
	class PanelMeterBar: public PanelControl
	{
	public:
		static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);
		virtual ~PanelMeterBar();

		virtual void Load();
		virtual void Refresh();
		virtual void SetAttribute(const std::string &attribute, double value);
		virtual bool GetAttribute(const std::string &attribute, double &value) const;

	protected:
		PanelMeterBar(Panel &panel, const Spumoni::JSON &node);

		void Reset();

		Spiral::Core::SampleFeed *m_Feed;
		std::vector<float> m_Samples;
		Fl_VU_Meter *m_Meter;
		bool m_VU, m_Bypass;
		double m_Datum, m_Min, m_Max;
	};
}

#endif
