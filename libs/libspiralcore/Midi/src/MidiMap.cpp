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
#include "MidiMap.h"

namespace Spiral
{
namespace Core
{
	bool ControlChange(const MidiPacket &packet, int &channel, int &controller, int &value)
	{
		if ((packet.Status & 0xf0) != 0xb0 || packet.Data1 > 127 || packet.Data2 > 127)
			return false;
		channel = (packet.Status & 0x0f) + 1;
		controller = packet.Data1;
		value = packet.Data2;
		return true;
	}

	const MidiDeviceDescription *MidiMap::Find(const std::string &device) const
	{
		for (size_t d = 0; d < Devices.size(); ++d)
			if (Devices[d].Device == device)
				return &Devices[d];
		return 0;
	}

	MidiDeviceDescription &MidiMap::Add(const std::string &device)
	{
		for (size_t d = 0; d < Devices.size(); ++d)
			if (Devices[d].Device == device)
				return Devices[d];
		Devices.push_back(MidiDeviceDescription(device));
		return Devices.back();
	}

	bool MidiMap::Remove(const std::string &device)
	{
		for (size_t d = 0; d < Devices.size(); ++d)
			if (Devices[d].Device == device)
			{
				Devices.erase(Devices.begin() + d);
				return true;
			}
		return false;
	}

	const MidiBindingDescription *MidiMap::Binding(const std::string &device, const std::string &control) const
	{
		const MidiDeviceDescription *listed = Find(device);
		for (size_t b = 0; listed && b < listed->Bindings.size(); ++b)
			if (listed->Bindings[b].Control == control)
				return &listed->Bindings[b];
		return 0;
	}

	void MidiMap::Bind(const std::string &device, const MidiBindingDescription &binding)
	{
		MidiDeviceDescription &listed = Add(device);
		for (size_t b = 0; b < listed.Bindings.size(); ++b)
			if (listed.Bindings[b].Control == binding.Control)
			{
				listed.Bindings[b] = binding;
				return;
			}
		listed.Bindings.push_back(binding);
	}

	bool MidiMap::Unbind(const std::string &device, const std::string &control)
	{
		for (size_t d = 0; d < Devices.size(); ++d)
		{
			if (Devices[d].Device != device)
				continue;
			std::vector<MidiBindingDescription> &bindings = Devices[d].Bindings;
			for (size_t b = 0; b < bindings.size(); ++b)
				if (bindings[b].Control == control)
				{
					bindings.erase(bindings.begin() + b);
					return true;
				}
		}
		return false;
	}

	void MidiMap::Match(int channel, int controller, std::vector<Target> &out) const
	{
		for (size_t d = 0; d < Devices.size(); ++d)
			for (size_t b = 0; b < Devices[d].Bindings.size(); ++b)
			{
				const MidiBindingDescription &binding = Devices[d].Bindings[b];
				if (binding.Controller != controller || (binding.Channel && binding.Channel != channel))
					continue;
				Target target;
				target.Device = Devices[d].Device;
				target.Control = binding.Control;
				out.push_back(target);
			}
	}
}
}
