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
#include "PanelMeterBar.h"
#include "Panel.h"
#include "DeviceSource.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "SampleFeed.h"
#include "JSON.h"
#include "Fl_VU_Meter.h"
#include <FL/Fl.H>
#include <cmath>
#include <cstdlib>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("MeterBar", &PanelMeterBar::Create);

	PanelControl *PanelMeterBar::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelMeterBar(panel, node);
	}

	PanelMeterBar::PanelMeterBar(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_Feed(NULL),
		m_VU(true),
		m_Bypass(false),
		m_Datum(0)
	{
		const Spumoni::JSON *samples = node.Get("samples");
		if (samples)
		{
			bool output = false;
			int port = (int)NumberOf(samples->Get("shadow"), 0);
			const std::string named = TextOf(samples->Get("port"));
			if (named.compare(0, 3, "in:") == 0)
				port = std::atoi(named.c_str() + 3);
			else if (named.compare(0, 4, "out:") == 0)
			{
				output = true;
				port = std::atoi(named.c_str() + 4);
			}
			m_Feed = m_Panel.Source()->RequestFeed(output, port);
		}
		m_Meter = new Fl_VU_Meter(m_X, m_Y, m_W, m_H);
		m_Meter->color(fl_rgb_color(20, 60, 20));
		m_Meter->vu_mode(true);
		Reset();
		Adopt(m_Meter);
	}

	PanelMeterBar::~PanelMeterBar()
	{
		m_Panel.Source()->ReleaseFeed(m_Feed);
	}

	void PanelMeterBar::Reset()
	{
		m_Min = 10;
		m_Max = -10;
	}

	void PanelMeterBar::Load()
	{
		if (!Property())
			return;
		const bool vu = Physical() != 0;
		if (vu == m_VU && m_Meter->vu_mode() == vu)
			return;
		if (vu != m_VU)
			Reset();
		m_VU = vu;
		m_Meter->vu_mode(vu);
		m_Meter->redraw();
	}

	void PanelMeterBar::Refresh()
	{
		Load();
		if (m_Bypass || !m_Feed || !m_Feed->Take(m_Samples))
			return;
		float datum = 0;
		for (size_t i = 0; i < m_Samples.size(); ++i)
		{
			datum = m_Samples[i];
			if (m_VU)
				datum = std::fabs(datum);
			if (datum < m_Min) m_Min = datum;
			if (datum > m_Max) m_Max = datum;
		}
		m_Datum = datum;
		if (m_Meter->value() != (float)datum)
			m_Meter->value(datum);
	}

	void PanelMeterBar::SetAttribute(const std::string &attribute, double value)
	{
		if (attribute == "bypass")
			m_Bypass = value != 0;
		else if (attribute == "reset")
			Reset();
	}

	bool PanelMeterBar::GetAttribute(const std::string &attribute, double &value) const
	{
		if (attribute == "datum") value = m_Datum;
		else if (attribute == "min") value = m_Min;
		else if (attribute == "max") value = m_Max;
		else return false;
		return true;
	}
}
