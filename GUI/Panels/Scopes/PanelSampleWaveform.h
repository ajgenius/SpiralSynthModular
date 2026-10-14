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
#ifndef SPIRALPANELS_PANELSAMPLEWAVEFORM_H
#define SPIRALPANELS_PANELSAMPLEWAVEFORM_H

#include "PanelControl.h"
#include <string>
#include <vector>

class Fl_WaveDisplay;
class Fl_Widget;

namespace SpiralPanels
{
	class PanelSampleWaveform: public PanelControl
	{
	public:
		static PanelControl *Create(Panel &panel, const Spumoni::JSON &node);

		virtual void Load();
		virtual void Refresh();
		virtual void SetAttribute(const std::string &attribute, double value);
		virtual bool GetAttribute(const std::string &attribute, double &value) const;

		size_t SampleCount() const { return m_Samples.size(); }

	protected:
		PanelSampleWaveform(Panel &panel, const Spumoni::JSON &node);

		void Reload();
		bool MarkersChanged();
		static void MarkerMoved(Fl_Widget *widget, void *data);

		Fl_WaveDisplay *m_Display;
		std::vector<float> m_Samples;
		const PanelProperty *m_Pos, *m_PlayStart, *m_LoopStart, *m_LoopEnd;
		std::string m_SampleCommand;
		std::string m_SizeChannel, m_BufferChannel;
		bool m_Fetched;
		long m_SentPlayStart, m_SentLoopStart, m_SentLoopEnd;
	};
}

#endif
