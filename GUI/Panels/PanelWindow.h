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
#ifndef SPIRAL_PANELS_PANEL_WINDOW_H
#define SPIRAL_PANELS_PANEL_WINDOW_H

#include "SpiralPluginGUI.h"
#include <map>
#include <string>

class Fl_Button;

namespace Spiral
{
	namespace Core
	{
		class StateValue;
		struct DeviceClass;
	}
}

namespace Spiral
{
	namespace Panels
	{
		class Panel;
		class FltkPanel;

		class PanelWindow: public SpiralPluginGUI
		{
		public:
			static PanelWindow *Create(SpiralPlugin *plugin, const Spiral::Core::DeviceClass &device);
			static void Provide();

			virtual void UpdateValues(SpiralPlugin *plugin);
			virtual void Update();

			Panel *GetPanel() const { return m_Panel; }
			void SaveState(std::map<std::string, Spiral::Core::StateValue> &state) const;
			void LoadState(const std::map<std::string, Spiral::Core::StateValue> &state);
			SpiralPlugin *Plugin() const;
			void SetLayout(size_t index);
			// Shows the mode button while the panel has modes to switch
			// between, which a panel its device writes may gain or lose.
			void OfferModes();
			void SetMode(size_t index);

		protected:
			PanelWindow(int w, int h, SpiralPlugin *plugin);
			static void NextLayout(Fl_Widget *widget, void *data);
			static void NextMode(Fl_Widget *widget, void *data);
			Fl_Button *m_Layout;
			Fl_Button *m_Mode;
			Fl_Button *m_Config;
			std::string m_ConfigControl;
			Fl_Widget *ConfigWidget() const;
			void LayoutHeader();
			static void OpenConfig(Fl_Widget *widget, void *data);

			static SpiralGUIType *CreateForDevice(SpiralPlugin *plugin, const Spiral::Core::DeviceClass &device);

			virtual const std::string GetHelpText(const std::string &loc);

			void Fit();
			static FltkPanel *Build(SpiralPlugin *plugin, const std::string &panel, const std::string &directory,
					const std::string &documentPath, bool &generated, std::string &config, bool whole = false);
			void Rebuild();
			FltkPanel *m_Host;
			Panel *m_Panel;
			std::string m_Document;
			std::string m_Directory;
			std::string m_DocumentPath;
			bool m_Generated;
			unsigned m_Generation;
		};
	}
}

#endif
