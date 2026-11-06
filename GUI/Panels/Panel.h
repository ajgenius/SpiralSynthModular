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
#ifndef SPIRAL_PANELS_PANEL_H
#define SPIRAL_PANELS_PANEL_H

#include "Binding.h"
#include "MemorySource.h"
#include "Handlers.h"
#include "ValueRule.h"
#include <FL/Fl_Group.H>
#include <map>
#include <string>
#include <vector>

class SpiralPlugin;

namespace Spiral
{
	namespace Core
	{
		class StateValue;
	}
}

namespace Spumoni
{
	class JSON;
}

namespace SpiralPanels
{
	class DeviceSource;
	class PanelControl;
	class PanelProperty;

	class Panel: public Fl_Group, public Spiral::Data::DataObserver
	{
	public:
		Panel(int x, int y, SpiralPlugin *plugin);
		virtual ~Panel();

		bool Build(const Spumoni::JSON &document, std::string *error = NULL);
		void BuildChildren(const Spumoni::JSON &children, int originX, int originY);

		void Load();
		void Refresh();
		virtual void DataChanged(Spiral::Data::DataSource &source, int field);
		virtual void DataSourceDestroyed(Spiral::Data::DataSource &source);
		bool Loading() const { return m_Loading; }

		const PanelProperty *FindProperty(const std::string &name) const;
		PanelControl *FindControl(const std::string &name) const;
		bool Write(const PanelProperty &property, double value);
		void WriteText(const PanelProperty &property, const std::string &text);
		bool Command(const std::string &name, const std::vector<std::string> &args = std::vector<std::string>());
		double Read(const PanelProperty &property) const;
		std::string ReadText(const PanelProperty &property) const;
		DeviceSource *Source() const { return m_Source; }
		bool IsState(const PanelProperty &property) const;
		void SaveState(std::map<std::string, Spiral::Core::StateValue> &state) const;
		void LoadState(const std::map<std::string, Spiral::Core::StateValue> &state);
		SpiralPlugin *Plugin() const;

		const std::string &Help() const { return m_Help; }

		size_t LayoutCount() const { return m_Layouts.size(); }
		const std::string &LayoutName(size_t index) const { return m_Layouts[index].Name; }
		size_t Layout() const { return m_Layout; }
		void ApplyLayout(size_t index);
		size_t ModeCount() const { return m_Modes.size(); }
		const std::string &ModeName(size_t index) const { return m_Modes[index].Name; }
		size_t Mode() const { return m_Mode; }
		void SelectMode(size_t index);
		const std::vector<std::string> &Unsupported() const { return m_Unsupported; }
		void Report(const std::string &problem) { m_Unsupported.push_back(problem); }
		bool Building() const { return m_Building; }
		class Scope: public Spiral::Data::ValueRule::Scope
		{
		public:
			explicit Scope(const Panel &panel): m_Panel(panel) {}
			bool HasField(const std::string &name) const;
			double Field(const std::string &name) const;
			bool HasAttribute(const std::string &control, const std::string &attribute) const;
			double Attribute(const std::string &control, const std::string &attribute) const;
			std::string Localize(const Spiral::Data::ValueRule::Text &text) const { return text.fallback; }
		private:
			const Panel &m_Panel;
		};
		class HandlerScope: public Spiral::Data::Handlers::Scope
		{
		public:
			HandlerScope(const Panel &panel, const PanelControl &control): m_Panel(panel), m_Control(control) {}
			bool HasField(const std::string &name) const;
			bool HasControl(const std::string &name) const;
			bool HasAttribute(const std::string &control, const std::string &attribute) const;
			bool CanSetAttribute(const std::string &control, const std::string &attribute) const;
			bool Event(const std::string &name, std::vector<std::string> &arguments) const;
		private:
			const Panel &m_Panel;
			const PanelControl &m_Control;
		};
		const std::vector<std::string> &Unbound() const { return m_Unbound; }
		size_t ControlCount() const { return m_Controls.size(); }
		PanelControl *Control(size_t index) const { return m_Controls[index]; }

	protected:
		void Clear();
		void BuildRepeat(const Spumoni::JSON &node, int originX, int originY);
		void ShowRepeats();

		struct Repeat
		{
			const PanelProperty *Count;
			std::vector<std::vector<Fl_Widget *> > Copies;
			int StepX, StepY;
			bool AutoWidth;
			int Shown;
		};
		int RepeatCount(const Repeat &repeat) const;
		std::vector<Repeat> m_Repeats;
		const Spumoni::JSON *m_AutoWidth;
		std::vector<Spumoni::JSON *> m_Owned;
		int m_Width;

		struct Bounds { int X, Y, W, H; };
		struct PanelLayout
		{
			std::string Name;
			int Width, Height;
			std::map<std::string, Bounds> Controls;
		};
		void ReadLayouts(const Spumoni::JSON &panel);
		std::vector<PanelLayout> m_Layouts;
		size_t m_Layout;
		struct PanelMode
		{
			std::string Name, Property;
			double Value;
		};
		void ReadModes(const Spumoni::JSON &panel);
		std::vector<PanelMode> m_Modes;
		size_t m_Mode;

		void Bind();
		Spiral::Data::DataSource &SourceOf(const PanelProperty &property) const;
		DeviceSource *m_Source;
		Spiral::Data::MemorySource *m_State;
		Spiral::Data::Binder m_Binder;
		std::map<std::string, PanelProperty *> m_Properties;
		std::vector<PanelControl *> m_Controls;
		std::vector<std::string> m_Unsupported;
		bool m_Building;
		std::vector<std::string> m_Unbound;
		std::string m_Help;
		bool m_Loading;
	};
}

#endif
