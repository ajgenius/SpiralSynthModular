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

#include "ClockPlugin.h"
#include "SpiralIcon.xpm"

using namespace Spiral::Core;
#include <config.h>

extern "C" {
const char *SpiralPlugin_GetHostVersion()
{
	return PACKAGE_VERSION;
}

const char *SpiralPlugin_GetHostABI()
{
	return SSM_HOST_ABI;
}

SpiralPlugin* SpiralPlugin_CreateInstance()
{
	return new ClockPlugin;
}

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DEVICE;
}

const char** SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}
}


ClockPlugin::ClockPlugin()
{
	m_Version = 1;
	m_PluginInfo.Name = "Clock";
	m_PluginInfo.Width = 80;
	m_PluginInfo.Height = 60;
	m_PluginInfo.NumInputs = 1;
	m_PluginInfo.NumOutputs = 4;
	m_PluginInfo.PortTips.push_back("Reset");
	m_PluginInfo.PortTips.push_back("Clock");
	m_PluginInfo.PortTips.push_back("Trigger");
	m_PluginInfo.PortTips.push_back("Note Clock");
	m_PluginInfo.PortTips.push_back("Tempo");
	SetString(DeviceName, "Clock");
	SetUnsigned(DeviceVersion, 1);
	m_Tempo = FloatProperty(DefaultLinearFlags, 120, 20, 300, 0.1f, 1);
	RegisterSharedProperty(m_Tempo, "BPM", "Tempo notes per minute");
	m_Beats = SignedProperty(DefaultLinearFlags, 4, 1, 32, 1, 1);
	RegisterSharedProperty(m_Beats, "Beats", "Beats in a bar: the time signature's upper number");
	m_BeatUnit = SignedProperty(DefaultLinearFlags, 4, 1, 32, 1, 1);
	RegisterSharedProperty(m_BeatUnit, "BeatUnit", "The note a beat is: the time signature's lower number");
	m_Steps = SignedProperty(DefaultLinearFlags, 4, 1, 32, 1, 1);
	RegisterSharedProperty(m_Steps, "Steps", "Steps in a beat: Clock changes level on each");
	m_Bars = SignedProperty(DefaultLinearFlags, 1, 1, 64, 1, 1);
	RegisterSharedProperty(m_Bars, "Bars", "Bars in a sequence: Trigger fires as each starts");
	m_TempoNote = SignedProperty(DefaultLinearFlags, 0, 0, 5, 1, 1);
	m_StepNote = SignedProperty(DefaultLinearFlags, 3, 0, 5, 1, 1);
	RegisterSharedProperty(m_TempoNote, "TempoNote", "Note value counted by BPM");
	RegisterSharedProperty(m_StepNote, "StepNote", "Note value of each Note Clock step");
}

void ClockPlugin::Apply(Description::Reader &reader)
{
	int version = 0;
	reader.Value(version).Value(m_Tempo).Value(m_Beats).Value(m_BeatUnit).Value(m_Steps).Value(m_Bars);
	SetSigned(m_TempoNote, 0);
	SetSigned(m_StepNote, 3);
	if (reader.More())
		reader.Value(m_TempoNote).Value(m_StepNote);
}

void ClockPlugin::Reset()
{
	SpiralPlugin::Reset();
	m_Grid.Reset();
}

void ClockPlugin::Execute()
{
	const unsigned long start = m_HostInfo->FRAME;
	m_Grid.Configure(start, m_HostInfo->SAMPLERATE, AsFloat(m_Tempo), AsSigned(m_TempoNote),
		AsSigned(m_Beats), AsSigned(m_BeatUnit), AsSigned(m_Steps), AsSigned(m_Bars), AsSigned(m_StepNote));
	const bool connected = InputExists(0);
	const bool rolling = m_HostInfo->ROLLING;
	for (int n = 0; n < m_HostInfo->BUFSIZE; ++n)
	{
		const bool trigger = m_Grid.Tick(rolling ? start + n : start, rolling, connected && GetInput(0, n) > 0);
		SetOutput(0, n, m_Grid.BeatLevel());
		SetOutput(1, n, trigger);
		SetOutput(2, n, m_Grid.NoteLevel());
		SetOutput(3, n, m_Grid.QuarterBPM());
	}
}
