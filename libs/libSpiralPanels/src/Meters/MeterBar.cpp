// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
#include "ControlSupport.h"
#include <cmath>

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_MeterBar("MeterBar", &MeterBarControl::Create);

		PanelControl *MeterBarControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new MeterBarControl(panel, node));
		}

		MeterBarControl::MeterBarControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Meter(MakeView<MeterView>("MeterBar", *this, node)),
			m_Feed(NULL),
			m_VU(true),
			m_Bypass(false),
			m_Datum(0),
			m_Low(10),
			m_High(-10)
		{
			if (!m_Meter)
				return;

			m_Feed = RequestFeed(m_Panel, node);
			m_Meter->SetMode(true);
			Adopt(m_Meter);
		}

		MeterBarControl::~MeterBarControl()
		{
			if (m_Feed)
				m_Panel.Source()->ReleaseFeed(m_Feed);
		}

		void MeterBarControl::Reset()
		{
			m_Low = 10;
			m_High = -10;
		}

		void MeterBarControl::Load()
		{
			if (!Property())
				return;

			const bool vu = Physical() != 0;

			if (vu == m_VU)
				return;

			Reset();
			m_VU = vu;
			m_Meter->SetMode(vu);
		}

		void MeterBarControl::Refresh()
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

				if (datum < m_Low)
					m_Low = datum;

				if (datum > m_High)
					m_High = datum;
			}

			m_Datum = datum;
			m_Meter->SetLevel(datum);
		}

		void MeterBarControl::SetAttribute(const std::string &attribute, double value)
		{
			if (attribute == "bypass")
				m_Bypass = value != 0;
			else if (attribute == "reset")
				Reset();
		}

		bool MeterBarControl::GetAttribute(const std::string &attribute, double &value) const
		{
			if (attribute == "datum")
				value = m_Datum;
			else if (attribute == "min")
				value = m_Low;
			else if (attribute == "max")
				value = m_High;
			else
				return false;

			return true;
		}
	}
}
