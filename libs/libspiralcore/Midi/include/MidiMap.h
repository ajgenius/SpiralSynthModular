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
#ifndef SPIRALCORE_MIDI_MAP_H
#define SPIRALCORE_MIDI_MAP_H

#include "MidiBackend.h"
#include "PatchDescription.h"
#include <string>
#include <vector>

#ifdef Status
#pragma push_macro("Status")
#undef Status
#define SSM_MIDI_MAP_RESTORE_STATUS
#endif

namespace Spiral
{
namespace Core
{
	/* A control change: its channel from 1 to 16, its controller number and
	   value. False for every other message. */
	bool ControlChange(const MidiPacket &packet, int &channel, int &controller, int &value);

	/* The patch's MIDI map while it runs: the devices it lists, by patch id,
	   and the controls a control change moves on each. Nothing is bound
	   until it is asked for. A control has one binding; a control change
	   may move any number of controls. The map is the file's, as is. */
	class MidiMap
	{
	public:
		struct Target
		{
			std::string Device, Control;
		};

		std::vector<MidiDeviceDescription> Devices;

		const MidiDeviceDescription *Find(const std::string &device) const;
		// Listed once, after those already listed.
		MidiDeviceDescription &Add(const std::string &device);
		bool Remove(const std::string &device);

		const MidiBindingDescription *Binding(const std::string &device, const std::string &control) const;
		// Lists the device if it is not, and replaces the control's binding.
		void Bind(const std::string &device, const MidiBindingDescription &binding);
		bool Unbind(const std::string &device, const std::string &control);

		// Each control a control change on this channel and number moves, in
		// the order they are listed.
		void Match(int channel, int controller, std::vector<Target> &out) const;
	};
}
}

#ifdef SSM_MIDI_MAP_RESTORE_STATUS
#pragma pop_macro("Status")
#undef SSM_MIDI_MAP_RESTORE_STATUS
#endif

#endif
