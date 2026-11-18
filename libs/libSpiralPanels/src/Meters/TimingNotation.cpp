// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
#include "ControlSupport.h"

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry registration("TimingNotation", &TimingNotationControl::Create);

		PanelControl *TimingNotationControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new TimingNotationControl(panel, node));
		}

		TimingNotationControl::TimingNotationControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Display(MakeView<TimingNotationView>("TimingNotation", *this, node)),
			m_Tempo(panel.FindProperty(TextOf(node.Get("tempo")))),
			m_Note(panel.FindProperty(TextOf(node.Get("tempo_note")))),
			m_Beats(panel.FindProperty(TextOf(node.Get("beats")))),
			m_Unit(panel.FindProperty(TextOf(node.Get("beat_unit"))))
		{
			if (m_Display)
				Adopt(m_Display);
		}

		void TimingNotationControl::Load()
		{
			if (m_Tempo && m_Note && m_Beats && m_Unit)
				m_Display->SetTiming(m_Panel.Read(*m_Tempo), int(m_Panel.Read(*m_Note)),
					int(m_Panel.Read(*m_Beats)), int(m_Panel.Read(*m_Unit)));
		}

		void TimingNotationControl::Refresh()
		{
			PanelControl::Refresh();
			Load();
		}
	}
}
