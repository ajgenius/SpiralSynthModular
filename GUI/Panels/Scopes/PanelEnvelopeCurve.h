// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef SPIRAL_PANELS_PANEL_ENVELOPE_CURVE_H
#define SPIRAL_PANELS_PANEL_ENVELOPE_CURVE_H

#include "PanelControl.h"
#include <string>

class Fl_Widget;

namespace SpiralPanels
{
	class EnvelopeCurveView;

	class PanelEnvelopeCurve: public PanelControl
	{
	public:
		static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);
		static std::string IndexedKey(const std::string &pattern, unsigned index);

		virtual void Load();
		virtual void Refresh();
		virtual void SetAttribute(const std::string &attribute, double value);

	protected:
		PanelEnvelopeCurve(Panel &panel, const Spumoni::JSON &node);

		static void Changed(Fl_Widget *widget, void *data);
		double ReadNamed(const std::string &name, double fallback) const;
		void WriteNamed(const std::string &name, double value);
		void WriteRow(unsigned index);
		void WriteAll();

		EnvelopeCurveView *m_View;
		std::string m_Count, m_Mode, m_Playhead, m_Length;
		std::string m_XPattern, m_YPattern, m_TangentXPattern, m_TangentYPattern;
		unsigned m_MaxCount;
		int m_Hold;
	};
}

#endif
