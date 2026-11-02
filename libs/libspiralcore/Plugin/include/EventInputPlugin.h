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

#ifndef EVENT_INPUT_PLUGIN
#define EVENT_INPUT_PLUGIN

#include <vector>
#include "SpiralPlugin.h"
#include "EventVoices.h"

class EventInputPlugin : public SpiralPlugin
{
public:
	static const Spiral::Core::DeviceClass &Class();

	EventInputPlugin();

	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void Reset();
	virtual void SetVoice(SpiralPlugin *first, unsigned voice, unsigned voices);

	virtual void Describe(Spiral::Core::Description &d) {}
	virtual void Apply(Spiral::Core::Description::Reader &r) {}

private:
	Spiral::Core::EventVoices m_Voices;
	EventInputPlugin *m_First;
	unsigned m_Voice, m_Count;
	const Spiral::Core::EventBuffer *m_Source;
	bool m_ResetEvents;
	std::vector<float> m_Pitch, m_Gate;
};

#endif
