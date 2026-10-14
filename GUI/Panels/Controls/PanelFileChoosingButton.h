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
#ifndef SPIRAL_PANELS_PANELFILECHOOSINGBUTTON_H
#define SPIRAL_PANELS_PANELFILECHOOSINGBUTTON_H

#include "PanelControl.h"
#include <string>
#include <utility>
#include <vector>

class Fl_Button;
class Fl_Widget;

namespace SpiralPanels
{
	class PanelFileChoosingButton: public PanelControl
	{
	public:
		typedef void (*Chooser)(void *user, bool save, const std::string &title, const std::string &filter,
			void (*complete)(void *request, const char *path), void *request);
		static void SetChooser(Chooser chooser, void *user);

		static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);
		virtual ~PanelFileChoosingButton();

		static bool ValidExtension(const std::string &extension);
		const std::string &Filename() const { return m_Filename; }
		const std::string &Extension() const { return m_Extension; }
		bool Save() const { return m_Save; }
		bool Pending() const { return m_Pending; }
		void Choose();

	protected:
		PanelFileChoosingButton(Panel &panel, const Spumoni::JSON &node);
		void Finish(const char *path);
		static void Clicked(Fl_Widget *, void *data);
		void DeclareEvents(std::vector<Event> &out) const;
		static void Complete(void *request, const char *path);

		struct Request;
		Fl_Button *m_Button;
		const PanelProperty *m_Target;
		bool m_Save, m_Pending;
		std::string m_Title, m_Extension, m_Filename;
		Request *m_Request;
	};
}

#endif
