// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
#include "ControlSupport.h"
#include <cstdio>
#include <limits>

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_SevenSegment("SevenSegment", &SevenSegmentControl::Create);

		PanelControl *SevenSegmentControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new SevenSegmentControl(panel, node));
		}

		SevenSegmentControl::SevenSegmentControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Display(NULL),
			m_Digits((int)NumberOf(node.Get("digits"), 1)),
			m_Time(TextOf(node.Get("format")) == "time"),
			m_Shown(std::numeric_limits<double>::quiet_NaN())
		{
			m_Display = MakeView<DigitsView>("SevenSegment", *this, node);

			if (!m_Display)
				return;

			Adopt(m_Display);
			Show(0);
		}

		void SevenSegmentControl::Load()
		{
			if (Property())
				Show(Value());
		}

		void SevenSegmentControl::SetAttribute(const std::string &attribute, double value)
		{
			if (attribute == "value")
				Show(value);
		}

		void SevenSegmentControl::Show(double value)
		{
			if (value == m_Shown)
				return;

			m_Shown = value;
			const size_t count = m_Digits > 0 ? (size_t)m_Digits : 0;

			if (m_Time)
			{
				const float t = value > 0 ? (float)value : 0;
				const int places[6] = {(int)(t / 600) % 10, (int)(t / 60) % 10, (int)(t / 10) % 6, (int)t % 10,
						(int)(t * 10) % 10, (int)(t * 100) % 10};

				for (size_t i = 0; i < count && i < 6; ++i)
					m_Display->SetDigit(i, places[i], i > 0 && i % 2 == 0 ? DigitsView::Colon : DigitsView::NoDot);

				return;
			}

			char text[64];
			std::snprintf(text, sizeof(text), "%1.5f", value);
			const char *c = text;

			for (size_t i = 0; i < count; ++i)
			{
				const bool dot = *c == '.';
				const DigitsView::Dot shown = dot ? DigitsView::Point : DigitsView::NoDot;

				if (*c == 0)
				{
					m_Display->SetDigit(i, 0, shown);

					continue;
				}

				if (dot)
					c++;

				m_Display->SetDigit(i, *c == '-' ? -1 : (int)*c - (int)'0', shown);
				c++;
			}
		}
	}
}
