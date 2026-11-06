// SPDX-License-Identifier: GPL-2.0-or-later
#include "Controls.h"
#include "ControlSupport.h"

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_LEDButton("LEDButton", &ButtonControl::CreateLED);

		PanelControl *ButtonControl::CreateLED(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new ButtonControl(panel, node, true));
		}
	}
}
