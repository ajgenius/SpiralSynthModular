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
#include "PanelFileChoosingButton.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Chooser.H>
#include <cctype>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("FileChoosingButton", &PanelFileChoosingButton::Create);

	static void FLTKChooser(void *, bool, const std::string &title, const std::string &filter,
		void (*complete)(void *, const char *), void *request)
	{
		complete(request, fl_file_chooser(title.c_str(), filter.empty() ? NULL : filter.c_str(), NULL));
	}

	static PanelFileChoosingButton::Chooser s_Chooser = FLTKChooser;
	static void *s_ChooserUser = NULL;

	void PanelFileChoosingButton::SetChooser(Chooser chooser, void *user)
	{
		s_Chooser = chooser ? chooser : FLTKChooser;
		s_ChooserUser = chooser ? user : NULL;
	}

	struct PanelFileChoosingButton::Request
	{
		PanelFileChoosingButton *Owner;
	};

	PanelControl *PanelFileChoosingButton::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelFileChoosingButton(panel, node);
	}

	bool PanelFileChoosingButton::ValidExtension(const std::string &extension)
	{
		if (extension.empty())
			return true;
		if (extension.size() < 2 || extension.size() > 16 || extension[0] != '.')
			return false;
		for (size_t i = 1; i < extension.size(); ++i)
			if (!std::isalnum((unsigned char)extension[i]))
				return false;
		return true;
	}

	PanelFileChoosingButton::PanelFileChoosingButton(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_Target(NULL),
		m_Save(false),
		m_Pending(false),
		m_Request(NULL)
	{
		const Spumoni::JSON *file = node.Get("file");
		if (file)
		{
			m_Target = panel.FindProperty(TextOf(file->Get("property")));
			const Spumoni::JSON *save = file->Get("save");
			m_Save = save && save->GetType() == Spumoni::JSON::Boolean && save->AsBool();
			m_Title = TextOf(file->Get("title"));
			m_Extension = TextOf(file->Get("extension"));
			if (!ValidExtension(m_Extension))
				m_Extension.clear();
		}
		if (m_Title.empty())
			m_Title = m_Save ? "Save file" : "Open file";
		m_Button = new Fl_Button(m_X, m_Y, m_W, m_H);
		m_Button->copy_label(TextOf(node.Get("text")).c_str());
		m_Button->labelsize((int)NumberOf(node.Get("size"), 10));
		if (TextOf(node.Get("box")) == "thin")
			m_Button->box(FL_THIN_UP_BOX);
		else
		{
			m_Button->box(FL_PLASTIC_UP_BOX);
			m_Button->color((Fl_Color)GUIColour());
			m_Button->selection_color((Fl_Color)GUIColour());
		}
		if (IsToggle())
			m_Button->type(FL_TOGGLE_BUTTON);
		m_Button->callback(Clicked, this);
		Adopt(m_Button);
	}

	PanelFileChoosingButton::~PanelFileChoosingButton()
	{
		if (m_Request)
			m_Request->Owner = NULL;
	}

	void PanelFileChoosingButton::Choose()
	{
		if (m_Pending)
			return;
		m_Pending = true;
		m_Request = new Request;
		m_Request->Owner = this;
		const std::string filter = m_Extension.empty() ? std::string() : "*" + m_Extension;
		s_Chooser(s_ChooserUser, m_Save, m_Title, filter, Complete, m_Request);
	}

	void PanelFileChoosingButton::Complete(void *data, const char *path)
	{
		Request *request = (Request *)data;
		PanelFileChoosingButton *owner = request->Owner;
		delete request;
		if (owner)
		{
			owner->m_Request = NULL;
			owner->Finish(path);
		}
	}

	void PanelFileChoosingButton::Finish(const char *path)
	{
		m_Pending = false;
		if (!path || !*path)
		{
			if (IsToggle())
			{
				m_Button->value(0);
				Fire("off", 0);
			}
			return;
		}
		std::string chosen = path;
		if (m_Save && !m_Extension.empty())
		{
			const size_t slash = chosen.find_last_of('/');
			const size_t dot = chosen.find_last_of('.');
			if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
				chosen += m_Extension;
		}
		m_Filename = chosen;
		if (m_Target)
			m_Panel.WriteText(*m_Target, m_Filename);
		Fire("filename_changed", 1, m_Filename);
	}

	void PanelFileChoosingButton::DeclareEvents(std::vector<Event> &out) const
	{
		static const Event chosen = {"filename_changed", {"value", "text", NULL}};
		static const Event off = {"off", {"value", "text", NULL}};
		PanelControl::DeclareEvents(out);
		out.push_back(chosen);
		out.push_back(off);
	}

	void PanelFileChoosingButton::Clicked(Fl_Widget *, void *data)
	{
		PanelFileChoosingButton *self = (PanelFileChoosingButton *)data;
		if (self->IsToggle() && !self->m_Button->value())
			self->Fire("off", 0);
		else
			self->Choose();
	}
}
