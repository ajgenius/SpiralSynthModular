/*  SpiralSound
 *  Copyleft (C) 2001 David Griffiths <dave@pawfal.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef OutputPLUGIN
#define OutputPLUGIN

#include "SpiralPlugin.h"
#include "AudioTransportHub.h"

class OutputPlugin : public AudioEndpoint
{
public:
	// Same values as AudioTransportHub::Mode; the GUI reads these.
	enum Mode {NO_MODE,INPUT,OUTPUT,DUPLEX,CLOSED};

	// Built into the host: registered with the device registry, no module.
	static const spiralcore::DeviceClass &Class();

	OutputPlugin();
	virtual ~OutputPlugin();

	virtual PluginInfo& Initialise(const HostInfo *Host);

	virtual void Execute();
	virtual void ExecuteCommands();

	virtual bool Kill();
	virtual void Reset();

	virtual bool IsAudioDriver() { return true; }
	// The hub moves the periods; this device only mixes in Execute.
	virtual AudioProcessType ProcessType() { return AudioDriver::NEVER; }
	virtual void ProcessAudio() {}
	virtual void ServiceAudio();
	virtual void TransportModeChanged(int mode) { m_ReportedMode=mode; }

	enum GUICommands {NONE, OPENREAD, OPENWRITE, OPENDUPLEX, CLOSE, SET_VOLUME, CLEAR_NOTIFY};
	float m_Volume;

	Mode GetMode() { return (Mode)m_Hub->GetMode(); }

	virtual void Describe(spiralcore::Description &d) {}
	virtual void Apply(spiralcore::Description::Reader &r) {}
private:
	AudioTransportHub *m_Hub;
	bool m_NotifyOpenOut;
	int m_ReportedMode;
	bool m_CheckedAlready;

	bool m_Registered;
};

#endif
