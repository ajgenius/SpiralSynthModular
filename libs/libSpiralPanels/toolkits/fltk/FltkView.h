// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_FLTK_VIEW_H
#define SPIRAL_PANELS_FLTK_VIEW_H

#include "ControlView.h"
#include <FL/Fl_Group.H>

class Fl_Widget;

namespace Spiral
{
	namespace Panels
	{
		void DrawFltkGear(const Fl_Widget &widget, int radius);

		class Panel;
		class PanelControl;
		class PanelSource;

		class FltkView: public virtual ControlView
		{
		public:
			explicit FltkView(Fl_Widget *widget): m_Widget(widget) {}

			Fl_Widget *Widget() const { return m_Widget; }
			static Fl_Widget *WidgetOf(const PanelControl &control);

			void Enable(bool enabled);
			bool Enabled() const;
			void Show(bool shown);
			bool Shown() const;
			void Tooltip(const std::string &text);
			void GetBounds(int &x, int &y, int &w, int &h) const;
			void SetBounds(int x, int y, int w, int h);
			bool Drivable() const;
			bool Drive(double amount, bool &drivenOn);

		private:
			Fl_Widget *m_Widget;
		};

		class FltkPanel: public Fl_Group, public PanelHost
		{
		public:
			FltkPanel(int x, int y, PanelSource *source);
			virtual ~FltkPanel();

			Panel &Model() { return *m_Panel; }
			const Panel &Model() const { return *m_Panel; }

			void Origin(int &x, int &y) const;
			int Width() const { return w(); }
			int Height() const { return h(); }
			void Resize(int w, int h);
			void Redraw() { redraw(); }
			void BeginBuild() { begin(); }
			void EndBuild() { end(); }
			void ClearViews() { clear(); }

		private:
			Panel *m_Panel;
		};
	}
}

#endif
