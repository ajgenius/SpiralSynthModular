// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_CONTROL_VIEW_H
#define SPIRAL_PANELS_CONTROL_VIEW_H

#include <string>

namespace Spiral
{
	namespace Panels
	{
		class ControlView
		{
		public:
			virtual ~ControlView() {}

			virtual void Enable(bool enabled) = 0;
			virtual bool Enabled() const = 0;
			virtual void Show(bool shown) = 0;
			virtual bool Shown() const = 0;
			virtual void Tooltip(const std::string &text) = 0;
			virtual void GetBounds(int &x, int &y, int &w, int &h) const = 0;
			virtual void SetBounds(int x, int y, int w, int h) = 0;
			virtual bool Drivable() const = 0;
			virtual bool Drive(double amount, bool &drivenOn) = 0;
		};

		class PanelHost
		{
		public:
			virtual ~PanelHost() {}

			virtual void Origin(int &x, int &y) const = 0;
			virtual int Width() const = 0;
			virtual int Height() const = 0;
			virtual void Resize(int w, int h) = 0;
			virtual void Redraw() = 0;
			virtual void BeginBuild() = 0;
			virtual void EndBuild() = 0;
			virtual void ClearViews() = 0;
		};
	}
}

#endif
