// SPDX-License-Identifier: GPL-2.0-or-later
#include "Input.h"
#include "EnvelopeCurveView.h"
#include "FltkTheme.h"
#include "FltkView.h"
#include "JSON.h"

namespace Spiral
{
	namespace Panels
	{
		namespace
		{
			class FltkCurve: public FltkView, public virtual CurveView
			{
			public:
				explicit FltkCurve(EnvelopeCurveView *view): FltkView(view), m_View(view) {}

				void Redraw() { m_View->redraw(); }

				void PlotSize(double &w, double &h) const
				{
					w = m_View->PlotWidth();
					h = m_View->PlotHeight();
				}

			private:
				EnvelopeCurveView *m_View;
			};

			ControlView *MakeEnvelopeCurve(PanelControl &control, const Spumoni::JSON &node)
			{
				return new FltkCurve(new EnvelopeCurveView(control.X(), control.Y(), control.W(), control.H(),
					static_cast<EnvelopeCurveControl &>(control), FltkTheme::For(control, "EnvelopeCurve", node)));
			}

			ViewRegistry::Entry s_EnvelopeCurve("EnvelopeCurve", &MakeEnvelopeCurve);
		}
	}
}
