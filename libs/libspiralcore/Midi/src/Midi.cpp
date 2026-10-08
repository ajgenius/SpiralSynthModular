/*
 *  Copyleft (C) 2000 David Griffiths <dave@pawfal.org>
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

#include "Midi.h"
#include "unistd.h"
#include "sys/types.h"
#include "signal.h"
#include "pthread.h"

using namespace spiralcore;

static const int MIDI_SCANBUFSIZE=256;
static const int MIDI_KEYOFFSET=0;

static const unsigned char STATUS_START            = 0x80;
static const unsigned char STATUS_NOTE_OFF         = 0x80;
static const unsigned char STATUS_NOTE_ON          = 0x90;
static const unsigned char STATUS_AFTERTOUCH       = 0xa0;
static const unsigned char STATUS_CONTROL_CHANGE   = 0xb0;
static const unsigned char STATUS_PROG_CHANGE      = 0xc0;
static const unsigned char STATUS_CHANNEL_PRESSURE = 0xd0;
static const unsigned char STATUS_PITCH_WHEEL      = 0xe0;
static const unsigned char STATUS_END              = 0xf0;
static const unsigned char SYSEX_START             = 0xf0;
static const unsigned char SYSEX_TERMINATOR        = 0xf7;
static const unsigned char MIDI_CLOCK              = 0xf8;
static const unsigned char ACTIVE_SENSE            = 0xfe;

static int NKEYS = 30;

MidiDevice *MidiDevice::m_Singleton;
string MidiDevice::m_AppName;
string MidiDevice::m_DeviceName;
string MidiDevice::m_WantedBackend;

void MidiDevice::Init(const string &name, Type t)
{
	if (!m_Singleton)
	{
		m_AppName=name;
		m_Singleton=new MidiDevice(t);
	}
}

void MidiDevice::SetBackendName(const string &name)
{
	m_WantedBackend=name;
	if (m_Singleton && m_Singleton->m_BackendName!=name)
	{
		pthread_mutex_lock(m_Singleton->m_Mutex);
		m_Singleton->CloseBackend();
		m_Singleton->OpenBackend();
		pthread_mutex_unlock(m_Singleton->m_Mutex);
	}
}

void MidiDevice::SetDeviceName(string s)
{
	m_DeviceName=s;
	if (m_Singleton && m_Singleton->m_Backend)
		m_Singleton->m_Backend->Select(s,s);
}

MidiDevice::MidiDevice(Type t) :
m_Poly (1),
m_Clock (1.0f),
m_ClockCount (0),
m_Backend (NULL)
{
     m_Mutex = new pthread_mutex_t;
     pthread_mutex_init (m_Mutex, NULL);
     OpenBackend();
}

MidiDevice::~MidiDevice() {
     pthread_mutex_lock (m_Mutex);
     CloseBackend();
     pthread_mutex_unlock (m_Mutex);
     pthread_mutex_destroy (m_Mutex);
     delete m_Mutex;
}

// The named backend, else the first registered that is not the dummy,
// else the dummy; connected to whatever device name is set.
void MidiDevice::OpenBackend()
{
	MidiBackendRegistry *registry=MidiBackendRegistry::Get();
	m_BackendName=m_WantedBackend;
	if (m_BackendName.empty() || !registry->Find(m_BackendName))
	{
		vector<string> names=registry->Names();
		m_BackendName=names.empty() ? "" : names.front();
	}
	m_Backend=registry->Create(m_BackendName);
	if (!m_Backend)
	{
		cerr<<"MidiDevice: no midi backend"<<endl;
		return;
	}
	cerr<<"MidiDevice: using "<<m_BackendName<<" backend"<<endl;
	m_Backend->Select(m_DeviceName,m_DeviceName);
}

void MidiDevice::CloseBackend()
{
	if (m_Backend) MidiBackendRegistry::Get()->Destroy(m_BackendName,m_Backend);
	m_Backend=NULL;
	for (int n=0; n<16; n++)
		while (!m_EventVec[n].empty()) m_EventVec[n].pop();
}

string MidiDevice::GetStatus()
{
	pthread_mutex_lock(m_Mutex);
	string status=m_Backend ? m_Backend->Status() : "no midi backend";
	pthread_mutex_unlock(m_Mutex);
	return status;
}

// returns the next event off the list, or an
// empty event if the list is exhausted
MidiEvent MidiDevice::GetEvent(int Device)
{
	if (Device<0 || Device>15)
	{
		cerr<<"GetEvent: Invalid Midi device "<<Device<<endl;
		return MidiEvent(MidiEvent::NONE,0,0);
	}

	pthread_mutex_lock(m_Mutex);
	CollectEvents();
	if (m_EventVec[Device].size()==0)
	{
		pthread_mutex_unlock(m_Mutex);
		return MidiEvent(MidiEvent::NONE,0,0);
	}

	MidiEvent event(m_EventVec[Device].front());
	m_EventVec[Device].pop();
	pthread_mutex_unlock(m_Mutex);

	return event;
}

void MidiDevice::SendEvent (int Device, const MidiEvent &Event) {
	if (Device<0 || Device>15)
	{
		cerr<<"SendEvent: Invalid Midi device "<<Device<<endl;
		return;
	}

	MidiPacket message;
	message.Data1=Event.GetNote()+MIDI_KEYOFFSET;
	message.Data2=(unsigned char)Event.GetVolume();

	if (Event.GetType()==MidiEvent::ON)
		message.Status=STATUS_NOTE_ON+Device;
	else if (Event.GetType()==MidiEvent::OFF)
		message.Status=STATUS_NOTE_OFF+Device;
	else
		return;

	pthread_mutex_lock(m_Mutex);
	if (m_Backend) m_Backend->Send(message);
	pthread_mutex_unlock(m_Mutex);
}

// collect events drains what the backend has polled since last time
// into the per channel lists; the clock is counted here as it comes.
// Called with the mutex held.
void MidiDevice::CollectEvents()
{
	if (!m_Backend) return;
	MidiPacket packet;
	for (int n=0; n<1024 && m_Backend->Poll(packet); n++)
	{
		if (packet.Status==MIDI_CLOCK)
		{
			m_ClockCount++;
			if (m_ClockCount==6)
			{
				m_Clock=-m_Clock;
				m_ClockCount=0;
			}
		}
		else if (packet.Status>=STATUS_START && packet.Status<STATUS_END)
			AddEvent(packet);
		// 0xff from the backend is a reset: the notes it knew are gone.
	}
}

// addevent converts the midi packet into midi message objects and
// stacks them onto the event list to be picked up by the app
void MidiDevice::AddEvent(const MidiPacket &packet)
{
	const unsigned char midi[3]={packet.Status,packet.Data1,packet.Data2};
	MidiEvent::type MessageType=MidiEvent::NONE;
	int Volume=0,Note=0,EventDevice=0;

	// note off
	if (midi[0] >= STATUS_NOTE_OFF && midi[0] < STATUS_NOTE_ON)
	{
		MessageType=MidiEvent::OFF;
		Note=midi[1]-MIDI_KEYOFFSET;
		EventDevice=midi[0]-STATUS_NOTE_OFF;
	}
	// note on
	else if (midi[0] >= STATUS_NOTE_ON && midi[0] < STATUS_AFTERTOUCH)
	{
		Volume = midi[2];

		// cope with Roland equipment, where note on's
		// with zero velocity are sent as note offs.
		if (Volume) MessageType=MidiEvent::ON;
		else MessageType=MidiEvent::OFF;

		Note=midi[1]-MIDI_KEYOFFSET;
		EventDevice=midi[0]-STATUS_NOTE_ON;
	}
	// aftertouch
	else if (midi[0] >= STATUS_AFTERTOUCH && midi[0] < STATUS_CONTROL_CHANGE)
	{
		MessageType=MidiEvent::AFTERTOUCH;
		Note=midi[1]-MIDI_KEYOFFSET;
		Volume=midi[2];
		EventDevice=midi[0]-STATUS_AFTERTOUCH;
	}
	// parameter
	else if (midi[0] >= STATUS_CONTROL_CHANGE && midi[0] < STATUS_PROG_CHANGE)
	{
		MessageType=MidiEvent::PARAMETER;
		Note=midi[1];
		Volume=midi[2];
		EventDevice=midi[0]-STATUS_CONTROL_CHANGE;
	}
	// channel pressure
	else if (midi[0] >= STATUS_CHANNEL_PRESSURE && midi[0] < STATUS_PITCH_WHEEL)
	{
		MessageType=MidiEvent::CHANNELPRESSURE;
		Volume=midi[1];
		EventDevice=midi[0]-STATUS_CHANNEL_PRESSURE;
	}
	// note pitchbend
	else if (midi[0] >= STATUS_PITCH_WHEEL && midi[0] < STATUS_END)
	{
		MessageType=MidiEvent::PITCHBEND;
		// should probably take the first byte into account too?
		Volume=midi[2];
		EventDevice=midi[0]-STATUS_PITCH_WHEEL;
	}

	if (EventDevice<0 || EventDevice>15)
	{
		cerr<<"Error - Midi device "<<EventDevice<<" ??"<<endl;
		return;
	}

	m_EventVec[EventDevice].push(MidiEvent(MessageType,Note,Volume));
}
