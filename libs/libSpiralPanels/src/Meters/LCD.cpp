// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
#include "ControlSupport.h"
#include <cstdio>

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_LCD("LCD", &LCDControl::Create);

		PanelControl *LCDControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new LCDControl(panel, node));
		}

		LCDControl::LCDControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Readout(MakeView<ReadoutView>("LCD", *this, node)),
			m_Format(TextOf(node.Get("format"))),
			m_Empty(TextOf(node.Get("placeholder")))
		{
			if (m_Format.empty())
				m_Format = "%1.5f";

			if (m_Readout)
				Adopt(m_Readout);
		}

		void LCDControl::Show(const std::string &text, int cursor)
		{
			if (cursor < 0 && text == m_Shown)
				return;

			m_Shown = text;
			m_Readout->SetReadout(text, cursor);
		}

		void LCDControl::Load()
		{
			if (!Property())
				return;

			if (Property()->GetKind() != PanelProperty::String)
			{
				SetAttribute("value", Value());

				return;
			}

			const std::string text = Text();
			Show(text.empty() ? m_Empty : text, text.empty() ? 0 : (int)text.size() - 1);
		}

		void LCDControl::SetAttribute(const std::string &attribute, double value)
		{
			if (attribute != "value")
				return;

			char text[64];
			std::snprintf(text, sizeof(text), m_Format.c_str(), value);
			Show(text, -1);
		}
	}
}
