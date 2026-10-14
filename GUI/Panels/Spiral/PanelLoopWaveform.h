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
#ifndef SPIRAL_PANELS_PANELLOOPWAVEFORM_H
#define SPIRAL_PANELS_PANELLOOPWAVEFORM_H

#include "PanelControl.h"
#include <string>
#include <vector>

class Fl_Loop;
class Fl_Trigger;
class Fl_Widget;

namespace SpiralPanels
{
	class PanelLoopWaveform: public PanelControl
	{
	public:
		static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

		virtual void Load();
		virtual void Refresh();
		virtual void SetAttribute(const std::string &attribute, double value);
		virtual bool GetAttribute(const std::string &attribute, double &value) const;

		size_t TriggerCount() const { return m_Triggers.size(); }
		size_t SampleCount() const { return m_Samples.size(); }

	protected:
		PanelLoopWaveform(Panel &panel, const Spumoni::JSON &node);

		void Reload();
		void ShowLength();
		Fl_Trigger *AddTrigger(int id, int channel, float angle);
		static std::vector<std::string> TriggerArgs(Fl_Trigger *trigger);
		const PanelProperty *TriggerProperty(const std::string &pattern, int index) const;
		void SyncTriggers();
		void NewTrigger();
		static void TriggerMoved(Fl_Widget *widget, void *data);

		Fl_Loop *m_Loop;
		std::vector<Fl_Trigger *> m_Triggers;
		std::vector<float> m_Samples;
		const PanelProperty *m_Pos, *m_Length;
		std::string m_SampleCommand;
		std::string m_SizeChannel, m_BufferChannel;
		const PanelProperty *m_TriggerCount;
		std::string m_TriggerChannels, m_TriggerTimes;
		std::string m_NewTriggerCommand;
		bool m_Fetched;
	};
}

#endif
