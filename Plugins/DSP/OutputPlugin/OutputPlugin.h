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
#include "OutputAudioClient.h"

class OutputPlugin : public AudioDriver
{
public:
	enum Mode {NO_MODE,INPUT,OUTPUT,DUPLEX,CLOSED};

	OutputPlugin();
	virtual ~OutputPlugin();

	virtual PluginInfo& Initialise(const HostInfo *Host);
	virtual SpiralGUIType*  CreateGUI();

	virtual void Execute();
	virtual void ExecuteCommands();

	virtual bool Kill();
	virtual void Reset();

	virtual bool IsAudioDriver() { return true; }
	virtual AudioProcessType ProcessType() { return AudioDriver::ALWAYS; }
	virtual void ProcessAudio();

	enum GUICommands {NONE, OPENREAD, OPENWRITE, OPENDUPLEX, CLOSE, SET_VOLUME, CLEAR_NOTIFY};
	float m_Volume;

	Mode GetMode() { return m_Mode; }

	virtual void StreamOut(std::ostream &s) {}
	virtual void StreamIn(std::istream &s)  {}
private:
	static std::vector<OutputPlugin *> m_Members;
	static bool m_Configured;
	void OpenMode(Mode mode);
	static Mode m_Mode;
	bool m_NotifyOpenOut;
	int m_ReportedMode;
	void ReportMode();
	bool m_CheckedAlready;
};

#endif
