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
#include "PanelFunctionPlot.h"
#include "Panel.h"
#include "DeviceSource.h"
#include "PanelControlRegistry.h"
#include "JSON.h"
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("FunctionPlot", &PanelFunctionPlot::Create);

	namespace
	{
		class PlotView: public Fl_Widget
		{
		public:
			PlotView(int x, int y, int w, int h, const std::vector<float> &values):
				Fl_Widget(x, y, w, h), m_Values(values)
			{
				box(FL_UP_BOX);
				color(fl_rgb_color(20, 60, 20));
			}

			virtual void draw()
			{
				const int ox = x() + 3, oy = y() + 3, ww = w() - 6, hh = h() - 6;
				draw_box();
				fl_push_clip(ox, oy, ww, hh);
				for (int i = -5; i <= 5; ++i)
				{
					fl_color(i == 0 ? fl_rgb_color(203, 255, 0) : fl_rgb_color(155, 155, 50));
					fl_line(ox, oy + hh / 2 + i * hh / 10, ox + ww, oy + hh / 2 + i * hh / 10);
					fl_line(ox + i * ww / 10 + ww / 2, oy, ox + i * ww / 10 + ww / 2, oy + hh);
				}
				if (m_Values.size() > 1)
				{
					fl_color(fl_rgb_color(100, 200, 100));
					fl_begin_line();
					for (size_t i = 0; i < m_Values.size(); ++i)
					{
						float v = m_Values[i];
						v = v > 1 ? 1 : v < -1 ? -1 : v;
						fl_vertex(ox + (double)i * ww / (m_Values.size() - 1), oy + hh / 2 - v * hh / 2);
					}
					fl_end_line();
				}
				fl_pop_clip();
			}

		private:
			const std::vector<float> &m_Values;
		};
	}

	PanelControl *PanelFunctionPlot::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelFunctionPlot(panel, node);
	}

	PanelFunctionPlot::PanelFunctionPlot(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_Channel(node.Get("samples") ? TextOf(node.Get("samples")->Get("property")) : TextOf(node.Get("channel")))
	{
		m_View = new PlotView(m_X, m_Y, m_W, m_H, m_Values);
		Adopt(m_View);
	}

	void PanelFunctionPlot::Load()
	{
		std::vector<float> values;
		if (m_Panel.Source()->ReadFloats(m_Channel, values) && values != m_Values)
		{
			m_Values = values;
			m_View->redraw();
		}
	}

	void PanelFunctionPlot::Refresh()
	{
		Load();
	}
}
