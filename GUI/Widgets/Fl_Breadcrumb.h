// SPDX-License-Identifier: GPL-2.0-or-later
// The breadcrumb under the canvas: the way from the patch to the page shown,
// as the private tree's patch pages show it ("Patch > Tone > Filter bank").
// Each crumb opens its page; the chevron after one lists the subpatches on
// that page, the one on the way marked.
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
	// The patch's own name for the first crumb; empty reads "Patch".
	void SetRoot(const std::string &name) { m_Root = name; }
	// Built again when the page, a name on the way or the subpatches under
	// it change, so it is cheap to call every tick. Not from a crumb's own
	// callback: building again deletes the crumbs.
	void Refresh();

	static int Height() { return 20; }

private:
	void Rebuild();
	static void Crumb(Fl_Widget *widget, void *data);
	static void Chevron(Fl_Widget *widget, void *data);

	Fl_Canvas *m_Canvas;
	std::string m_Root, m_Shown;
	std::vector<int> m_Path;                    // the pages from the patch (-1) to the one shown
	std::vector<Fl_Widget *> m_Crumbs;          // one per page on the way
	std::vector<Fl_Widget *> m_Chevrons;        // one per page, or NULL
	std::vector<std::vector<int> > m_Children;  // the subpatches each chevron lists
};

#endif
