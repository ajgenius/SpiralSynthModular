// SPDX-License-Identifier: GPL-2.0-or-later
#include "Meters.h"
#include "ControlSupport.h"

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_Scope("Scope", &ScopeControl::Create);

		PanelControl *ScopeControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new ScopeControl(panel, node));
		}

		ScopeControl::ScopeControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Trace(MakeView<TraceView>("Scope", *this, node)),
			m_Feed(NULL),
			m_Attenuation(NumberOf(node.Get("attenuation"), 0)),
			m_TimeBase(NumberOf(node.Get("time_base"), 1))
		{
			if (!m_Trace)
				return;

			m_Feed = RequestFeed(m_Panel, node);
			m_Trace->SetTimeBase(m_TimeBase);
			m_Trace->SetAttenuation(m_Attenuation);
			Adopt(m_Trace);
		}

		ScopeControl::~ScopeControl()
		{
			if (m_Feed)
				m_Panel.Source()->ReleaseFeed(m_Feed);
		}

		void ScopeControl::Refresh()
		{
			if (m_Feed && m_Feed->Take(m_Samples))
				m_Trace->SetSamples(m_Samples);
		}

		void ScopeControl::SetAttribute(const std::string &attribute, double value)
		{
			if (attribute == "attenuation")
			{
				m_Attenuation = value;
				m_Trace->SetAttenuation(value);
			}
			else if (attribute == "time_base")
			{
				m_TimeBase = value;
				m_Trace->SetTimeBase(value);
			}
		}

		bool ScopeControl::GetAttribute(const std::string &attribute, double &value) const
		{
			if (attribute == "attenuation")
				value = m_Attenuation;
			else if (attribute == "time_base")
				value = m_TimeBase;
			else
				return false;

			return true;
		}
	}
}
