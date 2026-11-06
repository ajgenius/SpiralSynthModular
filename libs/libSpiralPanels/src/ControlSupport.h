// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_CONTROL_SUPPORT_H
#define SPIRAL_PANELS_CONTROL_SUPPORT_H

#include "Controls.h"
#include "JSON.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "PanelSource.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace Spiral
{
	namespace Panels
	{
		template <class View>
		static View *MakeView(const char *type, PanelControl &control, const Spumoni::JSON &node)
		{
			return dynamic_cast<View *>(ViewRegistry::Get().Create(type, control, node));
		}

		template <class Made>
		static PanelControl *Viewed(Made *made)
		{
			if (made->View())
				return made;

			delete made;

			return NULL;
		}

		static inline Feed *RequestFeed(Panel &panel, const Spumoni::JSON &node)
		{
			const Spumoni::JSON *samples = node.Get("samples");

			if (!samples || !panel.Source())
				return NULL;

			bool output = false;
			int port = (int)PanelControl::NumberOf(samples->Get("shadow"), 0);
			const std::string named = PanelControl::TextOf(samples->Get("port"));

			if (named.compare(0, 3, "in:") == 0)
				port = std::atoi(named.c_str() + 3);
			else if (named.compare(0, 4, "out:") == 0)
			{
				output = true;
				port = std::atoi(named.c_str() + 4);
			}

			return panel.Source()->RequestFeed(output, port);
		}

		static inline int IntAt(const std::vector<unsigned char> &bytes, size_t index)
		{
			int value = 0;

			if ((index + 1) * sizeof(int) <= bytes.size())
				std::memcpy(&value, &bytes[index * sizeof(int)], sizeof(value));

			return value;
		}
	}
}

#endif
