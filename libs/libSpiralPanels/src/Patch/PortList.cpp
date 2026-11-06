// SPDX-License-Identifier: GPL-2.0-or-later
#include "Patch.h"
#include "ControlSupport.h"
#include <cstring>

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_PortList("PortList", &PortListControl::Create);

		static const int NameSize = 256;

		static std::string NameAt(const std::vector<unsigned char> &bytes, size_t index)
		{
			if ((index + 1) * NameSize > bytes.size())
				return std::string();

			const char *name = (const char *)&bytes[index * NameSize];

			return std::string(name, strnlen(name, NameSize));
		}

		PanelControl *PortListControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new PortListControl(panel, node));
		}

		PortListControl::PortListControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Ports(NULL),
			m_Connected(panel.FindProperty(TextOf(node.Get("connected")))),
			m_NamesCommand(TextOf(node.Get("list"))),
			m_RefreshCommand(TextOf(node.Get("refresh")))
		{
			Read(m_Columns[0], node.Get("outputs"));
			Read(m_Columns[1], node.Get("inputs"));
			m_Ports = MakeView<PortsView>("PortList", *this, node);

			if (m_Ports)
				Adopt(m_Ports);
		}

		void PortListControl::Read(Column &column, const Spumoni::JSON *node)
		{
			if (!node)
				return;

			column.Count = m_Panel.FindProperty(TextOf(node->Get("count")));
			column.NamesCount = m_Panel.FindProperty(TextOf(node->Get("names_count")));
			column.Label = TextOf(node->Get("label"));
			column.Connections = TextOf(node->Get("connections"));
			column.Names = TextOf(node->Get("names"));
			column.Connect = TextOf(node->Get("connect"));
			column.Disconnect = TextOf(node->Get("disconnect"));
		}

		void PortListControl::Load()
		{
			Sync(0);
			Sync(1);

			if (!m_RefreshCommand.empty())
				m_Panel.Command(m_RefreshCommand);

			Show(0);
			Show(1);
		}

		void PortListControl::Sync(int index)
		{
			Column &column = m_Columns[index];

			if (!column.Count)
				return;

			const int count = (int)m_Panel.Read(*column.Count);

			if (count > m_Ports->PortCount(index))
				column.Shown.clear();

			m_Ports->SetPortCount(index, count, column.Label);
		}

		void PortListControl::Refresh()
		{
			Show(0);
			Show(1);
		}

		void PortListControl::Show(int index)
		{
			Column &column = m_Columns[index];

			if (!m_Panel.Source())
				return;

			std::vector<unsigned char> bytes;
			m_Panel.Source()->ReadBytes(column.Connections, bytes);

			if (bytes == column.Shown && !column.Shown.empty())
				return;

			column.Shown = bytes;

			for (int n = 0; n < m_Ports->PortCount(index); ++n)
			{
				const std::string name = NameAt(bytes, (size_t)n);
				m_Ports->SetPort(index, n, name, !name.empty());
			}
		}

		void PortListControl::Clicked(int columnIndex, int index, bool pressed)
		{
			Column &column = m_Columns[columnIndex];
			PanelSource *source = m_Panel.Source();
			const bool attached = m_Connected && m_Panel.Read(*m_Connected) != 0;

			if (pressed && attached && source)
			{
				m_Panel.Command(m_NamesCommand);
				source->Wait();
				std::vector<unsigned char> bytes;
				source->ReadBytes(column.Names, bytes);
				double counted = 0;

				if (column.NamesCount)
					source->ReadNow(*column.NamesCount, counted);

				std::vector<std::string> names;

				for (int n = 0; n < (int)counted && (size_t)(n + 1) * NameSize <= bytes.size(); ++n)
					names.push_back(NameAt(bytes, (size_t)n));

				const int choice = Dialogs::Get() ? Dialogs::Get()->ChooseFrom(names) : -1;

				if (choice >= 0)
				{
					std::vector<std::string> args;
					args.push_back(TextOfNumber(index));
					args.push_back(names[choice]);
					m_Panel.Command(column.Connect, args);
					source->Wait();
					m_Ports->SetPort(columnIndex, index, names[choice], true);

					return;
				}
			}

			m_Panel.Command(column.Disconnect, std::vector<std::string>(1, TextOfNumber(index)));

			if (source && attached)
				source->Wait();

			m_Ports->SetPort(columnIndex, index, std::string(), false);
		}
	}
}
