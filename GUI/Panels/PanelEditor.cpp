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
#include "PanelEditor.h"
#include "Panel.h"
#include "JSON.h"
#include "DeviceClassRegistry.h"
#include "EditorClassRegistry.h"
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <algorithm>
#include <iostream>
#include <sys/stat.h>

namespace SpiralPanels
{
	static const int TitleHeight = 14;
	static const int Margin = 2;

	PanelEditor::PanelEditor(int w, int h, SpiralPlugin *plugin):
		SpiralPluginGUI(w, h, plugin, plugin->GetChannelHandler()),
		m_Layout(NULL),
		m_Mode(NULL),
		m_Panel(NULL),
		m_Generated(false),
		m_Generation(0)
	{
	}

	Panel *PanelEditor::Build(SpiralPlugin *plugin, const std::string &documentPath, bool &generated)
	{
		std::string error, text;
		generated = plugin->GeneratePanel(documentPath, text);
		Spumoni::JSONOwner document(generated ? Spumoni::ParseJSONText(text, &error) : Spumoni::ParseJSON(documentPath.c_str(), &error));
		if (!document.get())
		{
			std::cerr << "PanelEditor: " << documentPath << ": " << error << std::endl;
			return NULL;
		}

		Panel *panel = new Panel(Margin, TitleHeight, plugin);
		if (!panel->Build(*document, &error))
		{
			std::cerr << "PanelEditor: " << documentPath << ": " << error << std::endl;
			delete panel;
			return NULL;
		}
		for (size_t i = 0; i < panel->Unsupported().size(); ++i)
			std::cerr << "PanelEditor: " << documentPath << ": no control registered for \""
				  << panel->Unsupported()[i] << "\"" << std::endl;
		for (size_t i = 0; i < panel->Unbound().size(); ++i)
			std::cerr << "PanelEditor: " << documentPath << ": unbound " << panel->Unbound()[i] << std::endl;

		panel->Load();
		return panel;
	}

	PanelEditor *PanelEditor::Create(SpiralPlugin *plugin, const std::string &documentPath)
	{
		if (!plugin)
			return NULL;
		bool generated = false;
		Panel *panel = Build(plugin, documentPath, generated);
		if (!panel)
			return NULL;
		PanelEditor *editor = new PanelEditor(panel->w() + 2 * Margin, panel->h() + TitleHeight + Margin, plugin);
		editor->m_Panel = panel;
		editor->m_DocumentPath = documentPath;
		editor->m_Generated = generated;
		editor->m_Generation = plugin->PanelGeneration();
		editor->add(panel);
		if (panel->ModeCount() > 1)
		{
			editor->m_Mode = new Fl_Button(editor->w() - (panel->LayoutCount() > 1 ? 38 : 24), 2, 10, 10, "@square");
			editor->m_Mode->labeltype(FL_ENGRAVED_LABEL);
			editor->m_Mode->labelsize(10);
			editor->m_Mode->box(FL_NO_BOX);
			editor->m_Mode->down_box(FL_NO_BOX);
			editor->m_Mode->callback(NextMode, editor);
			editor->m_Mode->copy_tooltip(("Panel: " + panel->ModeName(panel->Mode())).c_str());
			editor->add(editor->m_Mode);
		}
		if (panel->LayoutCount() > 1)
		{
			editor->m_Layout = new Fl_Button(editor->w() - 24, 2, 10, 10, "@-37>");
			editor->m_Layout->labeltype(FL_ENGRAVED_LABEL);
			editor->m_Layout->labelsize(10);
			editor->m_Layout->box(FL_NO_BOX);
			editor->m_Layout->down_box(FL_NO_BOX);
			editor->m_Layout->callback(NextLayout, editor);
			editor->m_Layout->copy_tooltip(("Layout: " + panel->LayoutName(0)).c_str());
			editor->add(editor->m_Layout);
		}
		editor->end();
		return editor;
	}

	SpiralGUIType *PanelEditor::CreateFromDocument(SpiralPlugin *plugin, const std::string &documentPath)
	{
		return Create(plugin, documentPath);
	}

	unsigned PanelEditor::RegisterDocuments(const std::string &root)
	{
		unsigned count = 0;
		Spiral::Core::EditorClassRegistry *editors = Spiral::Core::EditorClassRegistry::Get();
		const std::vector<Spiral::Core::DeviceClass *> &devices = Spiral::Core::DeviceClassRegistry::Get()->Classes();

		for (size_t i = 0; i < devices.size(); ++i)
		{
			const Spiral::Core::DeviceClass *device = devices[i];
			std::string document;
			if (device->PanelGenerated)
				document = root + "/" + editors->Subdirectory() + "/" + device->Name + "Plugin";
			else
			{
				const std::string directory = device->Directory();
				if (directory.empty())
					continue;
				document = directory + "/default-ui.json";
				struct stat info;
				if (stat(document.c_str(), &info) != 0 || !S_ISREG(info.st_mode))
					continue;
				std::string error;
				Spumoni::JSONOwner parsed(Spumoni::ParseJSON(document.c_str(), &error));
				const Spumoni::JSON *panel = parsed.get() ? parsed->Get("panel") : NULL;
				const Spumoni::JSON *children = panel ? panel->Get("children") : NULL;
				if (!children || children->GetType() != Spumoni::JSON::Array || children->Size() == 0)
					continue;
			}

			Spiral::Core::EditorClass editor;
			editor.ForDevice = device->ID;
			editor.Toolkit = editors->Find(device->ID) ? "panel" : "fltk";
			editor.Icon = device->Icon;
			editor.Document = document;
			editor.CreateFromDocument = CreateFromDocument;
			if (editors->Register(editor))
				++count;
		}
		return count;
	}

	void PanelEditor::UpdateValues(SpiralPlugin *)
	{
		if (m_Panel)
		{
			m_Panel->Load();
			Fit();
		}
	}

	void PanelEditor::Fit()
	{
		if (m_Panel && m_Panel->w() + 2 * Margin != w())
			Resize(m_Panel->w() + 2 * Margin, h());
		if (m_Layout)
			m_Layout->position(x() + w() - 24, y() + 2);
		if (m_Mode)
			m_Mode->position(x() + w() - (m_Layout ? 38 : 24), y() + 2);
	}

	void PanelEditor::SaveState(std::map<std::string, Spiral::Core::StateValue> &state) const
	{
		if (m_Panel)
			m_Panel->SaveState(state);
	}

	void PanelEditor::LoadState(const std::map<std::string, Spiral::Core::StateValue> &state)
	{
		if (m_Panel)
			m_Panel->LoadState(state);
	}

	void PanelEditor::SetLayout(size_t index)
	{
		if (!m_Panel || index >= m_Panel->LayoutCount())
			return;
		m_Panel->ApplyLayout(index);
		Resize(m_Panel->w() + 2 * Margin, m_Panel->h() + TitleHeight + Margin);
		if (m_Layout)
		{
			m_Layout->position(x() + w() - 24, y() + 2);
			m_Layout->copy_tooltip(("Layout: " + m_Panel->LayoutName(index)).c_str());
		}
		redraw();
	}

	void PanelEditor::SetMode(size_t index)
	{
		if (!m_Panel || index >= m_Panel->ModeCount())
			return;
		m_Panel->SelectMode(index);
		if (m_Mode)
			m_Mode->copy_tooltip(("Panel: " + m_Panel->ModeName(index)).c_str());
	}

	void PanelEditor::NextMode(Fl_Widget *, void *data)
	{
		PanelEditor *self = (PanelEditor *)data;
		if (self->m_Panel && self->m_Panel->ModeCount())
			self->SetMode((self->m_Panel->Mode() + 1) % self->m_Panel->ModeCount());
	}

	void PanelEditor::NextLayout(Fl_Widget *, void *data)
	{
		PanelEditor *self = (PanelEditor *)data;
		if (self->m_Panel && self->m_Panel->LayoutCount())
			self->SetLayout((self->m_Panel->Layout() + 1) % self->m_Panel->LayoutCount());
	}

	static bool OnScreen(const Fl_Widget *w)
	{
		if (!w->visible_r())
			return false;
		int X = w->x(), Y = w->y(), R = X + w->w(), B = Y + w->h();
		for (const Fl_Widget *p = w->parent(); p; p = p->parent())
		{
			const bool window = p->type() >= FL_WINDOW;
			X = std::max(X, window ? 0 : p->x());
			Y = std::max(Y, window ? 0 : p->y());
			R = std::min(R, window ? p->w() : p->x() + p->w());
			B = std::min(B, window ? p->h() : p->y() + p->h());
			if (window)
				break;
		}
		return X < R && Y < B;
	}

	void PanelEditor::Update()
	{
		if (m_Generated && m_Panel && m_Panel->Plugin() && m_Panel->Plugin()->PanelGeneration() != m_Generation)
			Rebuild();
		if (m_Panel && OnScreen(this))
		{
			m_Panel->Refresh();
			Fit();
		}
	}

	void PanelEditor::Rebuild()
	{
		SpiralPlugin *plugin = m_Panel->Plugin();
		m_Generation = plugin->PanelGeneration();
		bool generated = false;
		Panel *panel = Build(plugin, m_DocumentPath, generated);
		if (!panel)
			return;
		remove(m_Panel);
		delete m_Panel;
		m_Panel = panel;
		panel->position(x() + Margin, y() + TitleHeight);
		add(panel);
		Resize(panel->w() + 2 * Margin, panel->h() + TitleHeight + Margin);
		if (m_Layout)
			m_Layout->position(x() + w() - 24, y() + 2);
		if (m_Mode)
			m_Mode->position(x() + w() - (m_Layout ? 38 : 24), y() + 2);
		redraw();
	}

	const std::string PanelEditor::GetHelpText(const std::string &loc)
	{
		if (m_Panel && !m_Panel->Help().empty())
			return m_Panel->Help();
		return SpiralPluginGUI::GetHelpText(loc);
	}
}
