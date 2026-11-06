// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
#include "FltkTheme.h"
#include "FltkSupport.h"
#include "Fl_LED_Button.H"
#include "Fl_Loop.h"
#include "Fl_SevenSeg.H"
#include "Fl_Trigger.h"
#include "Fl_VU_Meter.h"
#include "Fl_WaveDisplay.H"
#include "JSON.h"
#include "ScopeView.h"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <cstring>

namespace Spiral
{
	namespace Panels
	{
		namespace
		{
			class PlotWidget: public Fl_Widget
			{
			public:
				PlotWidget(int x, int y, int w, int h, const FltkLook &look): Fl_Widget(x, y, w, h), m_Look(look)
				{
					look.Apply(this);
				}

				std::vector<float> Values;

				void draw()
				{
					const int ox = x() + 3;
					const int oy = y() + 3;
					const int ww = w() - 6;
					const int hh = h() - 6;
					draw_box();
					fl_push_clip(ox, oy, ww, hh);

					for (int i = -5; i <= 5; ++i)
					{
						fl_color(m_Look.Colour(i == 0 ? "indicator" : "marker"));
						fl_line(ox, oy + hh / 2 + i * hh / 10, ox + ww, oy + hh / 2 + i * hh / 10);
						fl_line(ox + i * ww / 10 + ww / 2, oy, ox + i * ww / 10 + ww / 2, oy + hh);
					}

					if (Values.size() > 1)
					{
						fl_color(m_Look.Colour("trace"));
						fl_begin_line();

						for (size_t i = 0; i < Values.size(); ++i)
						{
							float v = Values[i];
							v = v > 1 ? 1 : v < -1 ? -1 : v;
							fl_vertex(ox + (double)i * ww / (Values.size() - 1), oy + hh / 2 - v * hh / 2);
						}

						fl_end_line();
					}

					fl_pop_clip();
				}

			private:
				FltkLook m_Look;
			};

			class FltkPlot: public FltkView, public virtual PlotView
			{
			public:
				explicit FltkPlot(PlotWidget *plot): FltkView(plot), m_Plot(plot) {}

				void SetValues(const std::vector<float> &values)
				{
					m_Plot->Values = values;
					m_Plot->redraw();
				}

			private:
				PlotWidget *m_Plot;
			};
			ControlView *MakeFunctionPlot(PanelControl &control, const Spumoni::JSON &node)
			{
				return new FltkPlot(new PlotWidget(control.X(), control.Y(), control.W(), control.H(),
					FltkTheme::For(control, "FunctionPlot", node)));
			}

			ViewRegistry::Entry s_FunctionPlot("FunctionPlot", &MakeFunctionPlot);
		}
	}
}
