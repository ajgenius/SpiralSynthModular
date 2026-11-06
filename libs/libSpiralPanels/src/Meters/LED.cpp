// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
#include "ControlSupport.h"

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_LED("LED", &LEDControl::Create);

		PanelControl *LEDControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new LEDControl(panel, node));
		}

		LEDControl::LEDControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_LED(MakeView<CheckView>("LED", *this, node)),
			m_HasChoice(node.Get("choice") != NULL),
			m_Choice(NumberOf(node.Get("choice"), 0)),
			m_Latch(node.Get("latch") && node.Get("latch")->AsBool())
		{
			if (m_LED)
				Adopt(m_LED);
		}

		void LEDControl::Load()
		{
			if (!Property())
				return;

			const double value = Physical();

			if (!m_Latch)
				m_LED->SetChecked(m_HasChoice ? value == m_Choice : value != 0);
			else if (value != 0)
				m_LED->SetChecked(true);
		}

		void LEDControl::SetAttribute(const std::string &attribute, double)
		{
			if (attribute == "clear")
				m_LED->SetChecked(false);
		}

		void LEDControl::Changed(bool lit)
		{
			if (m_Latch)
				m_LED->SetChecked(false);
			else if (!m_HasChoice)
				Commit(lit ? 1 : 0);
			else if (lit)
				Commit(m_Choice);
		}
	}
}
