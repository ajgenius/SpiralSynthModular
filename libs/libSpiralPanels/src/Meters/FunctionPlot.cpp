// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
#include "ControlSupport.h"

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_FunctionPlot("FunctionPlot", &FunctionPlotControl::Create);

		PanelControl *FunctionPlotControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new FunctionPlotControl(panel, node));
		}

		FunctionPlotControl::FunctionPlotControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Plot(MakeView<PlotView>("FunctionPlot", *this, node)),
			m_Channel(node.Get("samples") ? TextOf(node.Get("samples")->Get("property")) : TextOf(node.Get("channel")))
		{
			if (m_Plot)
				Adopt(m_Plot);
		}

		void FunctionPlotControl::Load()
		{
			std::vector<float> values;

			if (m_Panel.Source() && m_Panel.Source()->ReadFloats(m_Channel, values) && values != m_Values)
			{
				m_Values = values;
				m_Plot->SetValues(m_Values);
			}
		}

		void FunctionPlotControl::Refresh()
		{
			Load();
		}
	}
}
