// SPDX-License-Identifier: GPL-2.0-or-later
#include "Fl_Breadcrumb.h"
#include "Fl_Canvas.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/fl_draw.H>
#include <cstdio>

namespace
{
	std::string Title(Fl_Canvas *canvas, int box)
	{
		const std::string title = canvas->BoxTitle(box);
		return title.empty() ? std::string("Group") : title;
	}

	class ChevronButton: public Fl_Menu_Button
	{
	public:
		ChevronButton(int x, int y, int w, int h, const char *label):
			Fl_Menu_Button(x, y, w, h, label) {}

	protected:
		virtual void draw()
		{
			draw_box(box(), color());
			draw_label();
		}
	};
}

Fl_Breadcrumb::Fl_Breadcrumb(int x, int y, int w, int h):
	Fl_Group(x, y, w, h),
	m_Canvas(NULL)
{
	box(FL_FLAT_BOX);
	end();
}

void Fl_Breadcrumb::Refresh()
{
	if (!m_Canvas)
		return;

	std::vector<int> path;
	for (int page = m_Canvas->Page(), depth = 0; depth < 64; ++depth)
	{
		path.insert(path.begin(), page);
		if (page == -1)
			break;
		page = m_Canvas->PageOf(page);
	}

	std::string shown = m_Root;
	char number[16];
	for (size_t step = 0; step < path.size(); ++step)
	{
		snprintf(number, sizeof(number), "|%d:", path[step]);
		shown += number;
		if (path[step] != -1)
			shown += m_Canvas->BoxTitle(path[step]);
		const std::vector<int> children = m_Canvas->BoxesOn(path[step]);
		for (size_t c = 0; c < children.size(); ++c)
		{
			snprintf(number, sizeof(number), ",%d=", children[c]);
			shown += number + m_Canvas->BoxTitle(children[c]);
		}
	}
	if (shown == m_Shown)
		return;
	m_Shown = shown;
	m_Path = path;
	Rebuild();
}

void Fl_Breadcrumb::Rebuild()
{
	clear();
	resizable(NULL);
	m_Crumbs.clear();
	m_Chevrons.clear();
	m_Children.clear();

	Fl_Group *current = Fl_Group::current();
	begin();
	int left = x() + 4;
	for (size_t step = 0; step < m_Path.size(); ++step)
	{
		const bool last = step + 1 == m_Path.size();
		const std::string label = step == 0 ? (m_Root.empty() ? std::string("Patch") : m_Root)
						  : Title(m_Canvas, m_Path[step]);
		fl_font(last ? FL_HELVETICA_BOLD : FL_HELVETICA, 11);
		const int width = (int)fl_width(label.c_str()) + 12;

		Fl_Button *crumb = new Fl_Button(left, y() + 2, width, h() - 4);
		crumb->copy_label(label.c_str());
		crumb->box(FL_FLAT_BOX);
		crumb->down_box(FL_FLAT_BOX);
		crumb->color(color());
		crumb->labelsize(11);
		crumb->labelfont(last ? FL_HELVETICA_BOLD : FL_HELVETICA);
		crumb->clear_visible_focus();
		crumb->callback(Crumb, this);
		m_Crumbs.push_back(crumb);
		left += width;

		const std::vector<int> children = m_Canvas->BoxesOn(m_Path[step]);
		m_Children.push_back(children);
		if (children.empty())
		{
			if (!last)
			{
				Fl_Box *arrow = new Fl_Box(left, y() + 2, 14, h() - 4, ">");
				arrow->labelsize(11);
				left += 14;
			}
			m_Chevrons.push_back(NULL);
			continue;
		}

		Fl_Menu_Button *chevron = new ChevronButton(left, y() + 2, 16, h() - 4, ">");
		chevron->box(FL_FLAT_BOX);
		chevron->down_box(FL_FLAT_BOX);
		chevron->color(color());
		chevron->labelsize(11);
		chevron->textsize(11);
		chevron->clear_visible_focus();
		chevron->tooltip("Subpatches here");
		const int next = last ? -2 : m_Path[step + 1];
		for (size_t c = 0; c < children.size(); ++c)
		{
			const int item = chevron->add("x", 0, Chevron, this,
				FL_MENU_RADIO | (children[c] == next ? FL_MENU_VALUE : 0));
			chevron->replace(item, Title(m_Canvas, children[c]).c_str());
		}
		m_Chevrons.push_back(chevron);
		left += 16;
	}
	end();
	Fl_Group::current(current);
	redraw();
}

void Fl_Breadcrumb::Crumb(Fl_Widget *widget, void *data)
{
	Fl_Breadcrumb *bar = static_cast<Fl_Breadcrumb *>(data);
	for (size_t i = 0; i < bar->m_Crumbs.size(); ++i)
		if (bar->m_Crumbs[i] == widget && bar->m_Canvas)
		{
			bar->m_Canvas->ShowPage(bar->m_Path[i]);
			return;
		}
}

void Fl_Breadcrumb::Chevron(Fl_Widget *widget, void *data)
{
	Fl_Breadcrumb *bar = static_cast<Fl_Breadcrumb *>(data);
	for (size_t i = 0; i < bar->m_Chevrons.size(); ++i)
		if (bar->m_Chevrons[i] == widget && bar->m_Canvas)
		{
			const int picked = static_cast<Fl_Menu_Button *>(widget)->value();
			if (picked >= 0 && picked < (int)bar->m_Children[i].size())
				bar->m_Canvas->ShowPage(bar->m_Children[i][picked]);
			return;
		}
}
