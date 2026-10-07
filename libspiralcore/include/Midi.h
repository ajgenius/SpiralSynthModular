/*  SpiralSynth
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

#ifndef SPIRALCORE_MIDI_H
#define SPIRALCORE_MIDI_H

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <limits.h>
#include <queue>
#include <string>
#include <pthread.h>
#include "config.h"
#include "MidiBackend.h"

using namespace std;

namespace spiralcore
{

class MidiEvent
{
public:
	enum type{NONE,ON,OFF,AFTERTOUCH,PARAMETER,CHANNELPRESSURE,PITCHBEND};

	MidiEvent() {m_Type=NONE;}
	MidiEvent(type t, int note, float v)
		{m_Type=t; m_Note=note; m_Volume=v;}

	type GetType() const {return m_Type;}
	float GetVolume() const {return m_Volume;}
	int GetNote() const {return m_Note;}
private:
	float m_Volume;
	type  m_Type;
	int   m_Note;
};

/* The synth's view of MIDI: events per channel, a clock, and sends. The
   bytes come and go through a MidiBackend from the registry, chosen by
   name; this class knows no hardware API. */
class MidiDevice
{
public:
	~MidiDevice();

	enum Type{READ,WRITE};

	static void Init(const string &name, Type t);
	// The backend by registry name; empty means the first one that is not
	// the dummy. Takes effect at Init, or at once on a running device.
	static void SetBackendName(const string &name);
	// What the backend connects to: an OSS device path, a port name.
	static void SetDeviceName(string s);
        static MidiDevice *Get()      { return m_Singleton; }
	static void PackUpAndGoHome() { if (m_Singleton) delete m_Singleton; m_Singleton=NULL; }

	MidiEvent GetEvent(int Device);
	void SendEvent(int Device,const MidiEvent &Event);
	void SetPoly(int s) { m_Poly=s; }
	float GetClock() { return m_Clock; }
	string GetBackendName() { return m_BackendName; }
	string GetStatus();

private:
	MidiDevice(Type t);
	void OpenBackend();
	void CloseBackend();
	void CollectEvents();
	void AddEvent(const MidiPacket &packet);

	int  m_Poly;
	float m_Clock;
	int   m_ClockCount;
	queue<MidiEvent> m_EventVec[16];
	static MidiDevice *m_Singleton;
	pthread_mutex_t* m_Mutex;
	static string m_AppName;
	static string m_DeviceName;
	static string m_WantedBackend;
	MidiBackend *m_Backend;
	string m_BackendName;
};

} // namespace spiralcore

#endif
