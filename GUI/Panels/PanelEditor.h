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
#ifndef SPIRAL_PANELS_PANEL_EDITOR_H
#define SPIRAL_PANELS_PANEL_EDITOR_H

#include "SpiralPluginGUI.h"
#include <string>

class Fl_Button;

namespace SpiralPanels
{
	class Panel;

	class PanelEditor: public SpiralPluginGUI
	{
	public:
		static PanelEditor *Create(SpiralPlugin *plugin, const std::string &documentPath);
		static unsigned RegisterDocuments(const std::string &root = std::string());

		virtual void UpdateValues(SpiralPlugin *plugin);
		virtual void Update();

		Panel *GetPanel() const { return m_Panel; }
		void SetLayout(size_t index);
		void SetMode(size_t index);

	protected:
		PanelEditor(int w, int h, SpiralPlugin *plugin);
		static void NextLayout(Fl_Widget *widget, void *data);
		static void NextMode(Fl_Widget *widget, void *data);
		Fl_Button *m_Layout;
		Fl_Button *m_Mode;

		static SpiralGUIType *CreateFromDocument(SpiralPlugin *plugin, const std::string &documentPath);

		virtual const std::string GetHelpText(const std::string &loc);

		void Fit();
		static Panel *Build(SpiralPlugin *plugin, const std::string &documentPath, bool &generated);
		void Rebuild();
		Panel *m_Panel;
		std::string m_DocumentPath;
		bool m_Generated;
		unsigned m_Generation;
	};
}

#endif
