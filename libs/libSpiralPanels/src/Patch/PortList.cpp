// SPDX-License-Identifier: GPL-2.0-or-later
#include "Patch.h"
#include "ControlSupport.h"
#include <cstring>
#include <cstdio>
#include <sstream>

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
			m_RefreshCommand(TextOf(node.Get("refresh"))),
			m_Routes(TextOf(node.Get("routes"))), m_Query(TextOf(node.Get("query"))),
			m_Add(TextOf(node.Get("add_route"))), m_Remove(TextOf(node.Get("remove_route"))),
			m_PendingPort(0), m_Ticket(0), m_Choosing(false)
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

			if (!m_RefreshCommand.empty() && m_Panel.Source())
				m_Panel.Source()->Command(m_RefreshCommand, std::vector<std::string>());

			Show(0);
			Show(1);
		}

		void PortListControl::Sync(int index)
		{
			Column &column = m_Columns[index];

			if (!column.Count)
				return;

			const int count = (int)m_Panel.Read(*column.Count);

			if (count == m_Ports->PortCount(index)) return;
			if (count > m_Ports->PortCount(index))
				column.Shown.clear();

			m_Ports->SetPortCount(index, count, column.Label);
		}

		void PortListControl::Refresh()
		{
			Sync(0);
			Sync(1);
			if (m_PendingPort) ReceiveRoutes();
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

		void PortListControl::ReceiveRoutes()
		{
			const int col = m_PendingPort > 0 ? 0 : 1;
			const int row = (m_PendingPort > 0 ? m_PendingPort : -m_PendingPort) - 1;
			if (!m_Connected || !m_Panel.Read(*m_Connected) || row >= m_Ports->PortCount(col))
			{
				m_PendingPort = 0;
				return;
			}
			PanelSource *source = m_Panel.Source();
			std::vector<unsigned char> bytes;
			if (!source || !source->ReadBytes(m_Routes, bytes) || bytes.empty()) return;
			std::istringstream reply(std::string(bytes.begin(), bytes.end()));
			std::string item;
			int port;
			unsigned serial;
			if (!std::getline(reply, item, char(0)) || std::sscanf(item.c_str(), "%d %u", &port, &serial) != 2 ||
			    port != m_PendingPort || serial != m_Ticket) return;
			m_PendingPort = 0;
			std::vector<std::string> peers, labels;
			std::string first;
			unsigned connected = 0;
			while (std::getline(reply, item, char(0)))
			{
				if (item.size() < 2) continue;
				const bool present = item[0] == '-';
				peers.push_back(item);
				labels.push_back((present ? "Remove " : "Add ") + item.substr(1));
				if (present && ++connected == 1) first = item.substr(1);
			}
			if (connected > 1) first += " (+" + TextOfNumber(connected - 1) + ")";
			m_Ports->SetPort(col, row, first, connected != 0);
			if (!m_Choosing || !Dialogs::Get()) return;
			if (labels.empty()) labels.push_back("No available peers");
			const int choice = Dialogs::Get()->ChooseFrom(labels);
			if (choice < 0 || size_t(choice) >= peers.size() || serial != m_Ticket) return;
			std::vector<std::string> args;
			args.push_back(TextOfNumber(port));
			args.push_back(TextOfNumber(++m_Ticket));
			args.push_back(peers[choice].substr(1));
			m_Choosing = false;
			// A user selection completes during refresh, when automatic panel writes are suppressed.
			if (source->Command(peers[choice][0] == '-' ? m_Remove : m_Add, args))
				m_PendingPort = port;
		}

		void PortListControl::Clicked(int columnIndex, int index, bool pressed)
		{
			Column &column = m_Columns[columnIndex];
			PanelSource *source = m_Panel.Source();
			const bool attached = m_Connected && m_Panel.Read(*m_Connected) != 0;

			if (!m_Query.empty())
			{
				const int port = columnIndex == 0 ? index + 1 : -index - 1;
				// A click opens a cable menu even when this port already has peers.
				const std::string old = NameAt(column.Shown, index);
				m_Ports->SetPort(columnIndex, index, old, !old.empty());
				std::vector<std::string> args;
				args.push_back(TextOfNumber(port));
				args.push_back(TextOfNumber(++m_Ticket));
				m_Choosing = true;
				m_PendingPort = attached && source && m_Panel.Command(m_Query, args) ? port : 0;
				return;
			}

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
				const std::string old = NameAt(column.Shown, index);
				m_Ports->SetPort(columnIndex, index, old, !old.empty());
				return;
			}

			m_Panel.Command(column.Disconnect, std::vector<std::string>(1, TextOfNumber(index)));

			if (source && attached)
				source->Wait();

			m_Ports->SetPort(columnIndex, index, std::string(), false);
		}
	}
}
