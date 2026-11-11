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

#include "Device.h"

namespace Spiral
{
namespace Core
{
	class MidiEvent;
}
}

#ifndef OscillatorPLUGIN
#define OscillatorPLUGIN

class MidiPlugin : public Spiral::Core::Device
{
public:
 	MidiPlugin();
	virtual ~MidiPlugin();

	virtual PluginInfo& Initialise(const HostInfo *Host);
	virtual void 		Execute();
	virtual void        ExecuteCommands();
	virtual void	    Describe(Spiral::Core::Description &d);
	virtual void	    Apply(Spiral::Core::Description::Reader &r);
	
	// has to be defined in the plugin	
	
	int  GetDeviceNum()      { return AsSigned(m_DeviceNum); }
	bool GetNoteCut()        { return AsBoolean(m_NoteCut); }
	bool GetContinuousNotes()        { return AsBoolean(m_ContinuousNotes); }
	
	enum GUICommands{NONE,ADDCONTROL,DELCONTROL};
	struct GUIArgs
	{
		int s;
		char Name[256];
	};
		
private:
	GUIArgs m_GUIArgs;
	
	// Read-outs for the panel: what the MIDI backend says of itself, and
	// the controllers that have outputs, by name.
	char m_Status[256];
	char m_Mapped[1024];
	long m_StatusAge;
	void TellStatus();
	void TellMapped();

	void AddControl(int s,const std::string &Name);
	void DeleteControl();
	void PlaceEvents();
	void SendEvents();
	void ReceiveEvent(const Spiral::Core::MidiEvent &In);
	
	Spiral::Core::SignedProperty m_DeviceNum;

	float m_NoteLevel;
	float m_TriggerLevel;
	float m_PitchBendLevel;
	float m_ChannelPressureLevel;
	float m_AfterTouchLevel;
	float m_ControlLevel[128];
	Spiral::Core::ToggleProperty  m_NoteCut;
	Spiral::Core::ToggleProperty  m_ContinuousNotes;
	int	  m_CurrentNote;
	struct Held
	{
		int ID;
		int Key;
	} m_Held[1024];
	int m_NextID;
	
	static int m_RefCount;
	static bool m_OpenedDevice;
	
	Spiral::Core::VectorProperty m_Controls;
	Spiral::Core::SignedProperty &Control(int n)
	{ return *(Spiral::Core::SignedProperty *)(*(Spiral::Core::VectorProperty *)m_Controls[n])[0]; }
	Spiral::Core::StringProperty &ControlName(int n)
	{ return *(Spiral::Core::StringProperty *)(*(Spiral::Core::VectorProperty *)m_Controls[n])[1]; }
};

#endif
