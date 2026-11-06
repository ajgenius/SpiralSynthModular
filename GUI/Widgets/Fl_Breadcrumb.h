// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef FL_BREADCRUMB_H
#define FL_BREADCRUMB_H

#include <FL/Fl_Group.H>
#include <string>
#include <vector>

class Fl_Canvas;

class Fl_Breadcrumb : public Fl_Group
{
public:
	Fl_Breadcrumb(int x, int y, int w, int h);

	void Bind(Fl_Canvas *canvas) { m_Canvas = canvas; }
	void SetRoot(const std::string &name) { m_Root = name; }
	void Refresh();

	static int Height() { return 20; }

private:
	void Rebuild();
	static void Crumb(Fl_Widget *widget, void *data);
	static void Chevron(Fl_Widget *widget, void *data);

	Fl_Canvas *m_Canvas;
	std::string m_Root, m_Shown;
	std::vector<int> m_Path;
	std::vector<Fl_Widget *> m_Crumbs;
	std::vector<Fl_Widget *> m_Chevrons;
	std::vector<std::vector<int> > m_Children;
};

#endif
