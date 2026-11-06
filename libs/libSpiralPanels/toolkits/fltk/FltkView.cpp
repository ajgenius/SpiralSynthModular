// SPDX-License-Identifier: GPL-2.0-or-later
#include "FltkView.h"
#include "Panel.h"
#include "PanelControl.h"
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Valuator.H>
#include <FL/Fl_Widget.H>
#include <algorithm>

namespace Spiral
{
	namespace Panels
	{
		Fl_Widget *FltkView::WidgetOf(const PanelControl &control)
		{
			const FltkView *view = dynamic_cast<const FltkView *>(control.View());

			return view ? view->Widget() : NULL;
		}

		void FltkView::Enable(bool enabled)
		{
			if (enabled)
				m_Widget->activate();
			else
				m_Widget->deactivate();
		}

		bool FltkView::Enabled() const
		{
			return m_Widget->active_r();
		}

		void FltkView::Show(bool shown)
		{
			if (shown)
				m_Widget->show();
			else
				m_Widget->hide();
		}

		bool FltkView::Shown() const
		{
			return m_Widget->visible_r();
		}

		void FltkView::Tooltip(const std::string &text)
		{
			m_Widget->copy_tooltip(text.c_str());
		}

		void FltkView::GetBounds(int &x, int &y, int &w, int &h) const
		{
			x = m_Widget->x();
			y = m_Widget->y();
			w = m_Widget->w();
			h = m_Widget->h();
		}

		void FltkView::SetBounds(int x, int y, int w, int h)
		{
			if (w == m_Widget->w() && h == m_Widget->h())
				m_Widget->position(x, y);
			else
				m_Widget->resize(x, y, w, h);
		}

		bool FltkView::Drivable() const
		{
			return dynamic_cast<Fl_Valuator *>(m_Widget) || dynamic_cast<Fl_Choice *>(m_Widget)
				|| dynamic_cast<Fl_Button *>(m_Widget);
		}

		bool FltkView::Drive(double amount, bool &drivenOn)
		{
			if (Fl_Valuator *valuator = dynamic_cast<Fl_Valuator *>(m_Widget))
			{
				const double low = valuator->minimum();
				const double high = valuator->maximum();
				const double value = valuator->clamp(valuator->round(low + amount * (high - low)));

				if (valuator->value(value))
					valuator->do_callback();

				return true;
			}

			if (Fl_Choice *choice = dynamic_cast<Fl_Choice *>(m_Widget))
			{
				const int items = choice->size() - 1;

				if (items <= 0)
					return false;

				const int item = std::min(items - 1, (int)(amount * items));

				if (choice->value() != item)
				{
					choice->value(item);
					choice->do_callback();
				}

				return true;
			}

			Fl_Button *button = static_cast<Fl_Button *>(m_Widget);
			const bool on = amount >= 0.5;
			const bool pressed = on && !drivenOn;
			drivenOn = on;

			switch (button->type())
			{
				case FL_TOGGLE_BUTTON:
					if (button->value() != on)
					{
						button->value(on);
						button->do_callback();
					}

					break;
				case FL_RADIO_BUTTON:
					if (on && !button->value())
					{
						button->setonly();
						button->do_callback();
					}

					break;
				default:
					if (pressed)
						button->do_callback();

					break;
			}

			return true;
		}

		FltkPanel::FltkPanel(int x, int y, PanelSource *source):
			Fl_Group(x, y, 1, 1),
			m_Panel(NULL)
		{
			end();
			m_Panel = new Panel(*this, source);
		}

		FltkPanel::~FltkPanel()
		{
			delete m_Panel;
		}

		void FltkPanel::Origin(int &x, int &y) const
		{
			x = this->x();
			y = this->y();
		}

		void FltkPanel::Resize(int w, int h)
		{
			if (w == this->w() && h == this->h())
				return;

			Fl_Widget::resize(x(), y(), w, h);

			if (parent())
				parent()->redraw();
		}
	}
}
