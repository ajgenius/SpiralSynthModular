// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_FLTK_SUPPORT_H
#define SPIRAL_PANELS_FLTK_SUPPORT_H

#include "Controls.h"
#include "FltkView.h"
#include "JSON.h"
#include <FL/Fl_Button.H>
#include <string>

namespace Spiral
{
	namespace Panels
	{
		static inline std::string Text(const Spumoni::JSON &node, const char *name)
		{
			return PanelControl::TextOf(node.Get(name));
		}

		static inline double Number(const Spumoni::JSON &node, const char *name, double fallback)
		{
			return PanelControl::NumberOf(node.Get(name), fallback);
		}

		class FltkButton: public FltkView, public virtual ButtonView
		{
		public:
			explicit FltkButton(Fl_Button *button): FltkView(button), m_Button(button) {}

			void SetText(const std::string &text) { m_Button->copy_label(text.c_str()); }
			void SetPressed(bool pressed) { m_Button->value(pressed); }

		private:
			Fl_Button *m_Button;
		};

		static inline void ButtonChanged(Fl_Widget *widget, void *data)
		{
			static_cast<ButtonControl *>(data)->Changed(static_cast<Fl_Button *>(widget)->value() != 0);
		}
	}
}

#endif
