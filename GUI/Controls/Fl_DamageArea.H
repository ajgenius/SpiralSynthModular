// Damage part of a widget so only that part is drawn again. A widget with
// something drawn after it there (a later sibling, or one of a group that
// holds it: another device box on the canvas) damages its window instead,
// so whatever is on top is drawn again over it, in order, rather than the
// widget alone on top.
#ifndef FL_DAMAGEAREA_H
#define FL_DAMAGEAREA_H

#include <FL/Fl_Group.H>
#include <FL/Fl_Window.H>

inline bool Fl_AreaCovered(const Fl_Widget *w, int X, int Y, int W, int H)
{
	for (const Fl_Widget *o = w; o->parent() && o->type() < FL_WINDOW; o = o->parent())
	{
		const Fl_Group *g = o->parent();
		for (int i = g->find(o) + 1; i < g->children(); ++i)
		{
			const Fl_Widget *s = g->child(i);
			if (s->visible() && s->x() < X + W && X < s->x() + s->w() && s->y() < Y + H && Y < s->y() + s->h())
				return true;
		}
	}
	return false;
}

inline void Fl_DamageArea(Fl_Widget *w, int X, int Y, int W, int H)
{
	if (w->window() && Fl_AreaCovered(w, X, Y, W, H))
		w->window()->damage(FL_DAMAGE_ALL, X, Y, W, H);
	else
		w->damage(FL_DAMAGE_ALL, X, Y, W, H);
}

// Damage what a box-less widget is drawn on: the nearest group that draws a
// box, over the widget, so that box and whatever else is there are drawn
// again in order, the widget among them. FLTK damages the window there
// instead, which draws everything in the window under it, every wire too.
inline void Fl_DamageBehind(Fl_Widget *w)
{
	Fl_Widget *o = w->parent();
	while (o && o->type() < FL_WINDOW && o->box() == FL_NO_BOX)
		o = o->parent();
	if (o)
		Fl_DamageArea(o, w->x(), w->y(), w->w(), w->h());
}

#endif
