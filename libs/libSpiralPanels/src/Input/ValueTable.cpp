// SPDX-License-Identifier: GPL-2.0-or-later
#include "Input.h"
#include "ControlSupport.h"

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_ValueTable("ValueTable", &ValueTableControl::Create);

		PanelControl *ValueTableControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new ValueTableControl(panel, node));
		}

		ValueTableControl::ValueTableControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Table(MakeView<TableView>("ValueTable", *this, node)),
			m_Values(TextOf(node.Get("values"))),
			m_Count(TextOf(node.Get("count"))),
			m_Pos(TextOf(node.Get("position"))),
			m_Bounded(node.Get("min") && node.Get("max")),
			m_Low((int)NumberOf(node.Get("min"), 0)),
			m_High((int)NumberOf(node.Get("max"), 0)),
			m_FirstLow((int)NumberOf(node.Get("first_min"), NumberOf(node.Get("min"), 0)))
		{
			if (m_Table)
				Adopt(m_Table);
		}

		void ValueTableControl::Load()
		{
			if (!m_Panel.Source())
				return;

			std::vector<unsigned char> values, count, pos;

			if (m_Panel.Source()->ReadBytes(m_Values, values))
				m_Table->SetValues(values);

			if (m_Panel.Source()->ReadBytes(m_Count, count))
				m_Table->SetRows(IntAt(count, 0));

			if (m_Panel.Source()->ReadBytes(m_Pos, pos))
				m_Table->SetPosition(IntAt(pos, 0));
		}

		void ValueTableControl::Refresh()
		{
			Load();
		}

		int ValueTableControl::Clamp(int row, int value) const
		{
			if (!m_Bounded)
				return value;

			const int least = row == 0 ? m_FirstLow : m_Low;

			return value < least ? least : (value > m_High ? m_High : value);
		}

		void ValueTableControl::Act(int row, int column, int value)
		{
			Spiral::Data::Handlers::Arguments arguments;
			arguments["row"] = Spiral::Data::ControlValue((double)row);
			arguments["column"] = Spiral::Data::ControlValue((double)column);
			arguments["value"] = Spiral::Data::ControlValue((double)value);
			Fire("cell_changed", arguments);
		}

		void ValueTableControl::DeclareEvents(std::vector<Event> &out) const
		{
			static const Event changed = {"cell_changed", {"row", "column", "value", NULL}};
			PanelControl::DeclareEvents(out);
			out.push_back(changed);
		}
	}
}
