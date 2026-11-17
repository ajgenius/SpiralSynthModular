// SPDX-License-Identifier: GPL-2.0-or-later
#include "Input.h"
#include "ControlSupport.h"
#include "RowSource.h"

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
			m_Bounded(node.Get("min") && node.Get("max")),
			m_Low((int)NumberOf(node.Get("min"), 0)),
			m_High((int)NumberOf(node.Get("max"), 0)),
			m_FirstLow((int)NumberOf(node.Get("first_min"), NumberOf(node.Get("min"), 0)))
		{
			if (m_Table)
				Adopt(m_Table);
		}

		// The table shows the rows of the source it names: a row to a line,
		// a field to a column, the source's cursor the line playing.
		void ValueTableControl::Load()
		{
			Spiral::Data::RowSource *rows = m_Panel.Rows(RowSourceName());

			if (!rows)
				return;

			const long count = rows->Count();
			const int columns = rows->FieldCount();
			std::vector<unsigned char> values((size_t)count * columns * sizeof(int));

			for (long row = 0; row < count; ++row)
			{
				for (int column = 0; column < columns; ++column)
				{
					Spiral::Data::ControlValue held;
					const int cell = rows->GetAt(row, column, held) ? (int)held.number : 0;

					std::memcpy(&values[((size_t)row * columns + column) * sizeof(int)], &cell, sizeof(cell));
				}
			}

			m_Table->SetValues(values);
			m_Table->SetRows((int)count);
			m_Table->SetPosition((int)rows->Position());
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
