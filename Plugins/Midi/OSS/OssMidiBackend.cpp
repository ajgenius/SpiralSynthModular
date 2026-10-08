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

// Dave's raw OSS device reader as a loadable backend: the device path is
// the selected source (and destination), opened non-blocking and read a
// byte at a time into packets by the worker, as the old reader thread
// did; writes go straight to the device.
#include "MidiBackend.h"
#include "NativeMidi.h"
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>

using namespace spiralcore;

class OssMidiTransport : public MidiTransport
{
	int m_MidiFd, m_MidiWrFd;
	std::string m_DeviceName, m_WriteName, m_Error;
	MidiByteStream m_Bytes;

public:
	OssMidiTransport() : m_MidiFd(-1), m_MidiWrFd(-1) {}

	void Open() {}
	void Close();
	bool Alive() const { return true; }
	std::vector<std::string> Ports(bool output);
	bool Connect(const std::string &source, const std::string &destination);
	bool Connected() { return m_MidiFd >= 0; }
	bool Read(MidiPacket &packet);
	bool Write(const MidiPacket &packet);
	std::string Status() const;
};

bool OssMidiTransport::Connect(const std::string &source, const std::string &destination)
{
	Close();
	m_DeviceName = source;
	m_WriteName = destination.empty() ? source : destination;
	m_Error = "";
	if (m_DeviceName.empty()) { m_Error = "no midi device named"; return false; }
	m_MidiFd = open (m_DeviceName.c_str(), O_RDONLY | O_NONBLOCK);
	if (m_MidiFd < 0) {
		m_Error = "Couldn't open midi for reading [" + m_DeviceName + "]: " + strerror(errno);
		return false;
	}
	m_MidiWrFd = open (m_WriteName.c_str(), O_WRONLY | O_NONBLOCK);
	if (m_MidiWrFd < 0) {
		m_Error = "Couldn't open midi for writing [" + m_WriteName + "]: " + strerror(errno);
	}
	return true;
}

void OssMidiTransport::Close() {
	if (m_MidiFd >= 0) close(m_MidiFd);
	if (m_MidiWrFd >= 0) close(m_MidiWrFd);
	m_MidiFd = m_MidiWrFd = -1;
}

std::vector<std::string> OssMidiTransport::Ports(bool output)
{
	std::vector<std::string> names;
	if (m_MidiFd >= 0) names.push_back(output ? m_WriteName : m_DeviceName);
	return names;
}

// a byte at a time, until a packet completes or the device has no more
bool OssMidiTransport::Read(MidiPacket &packet)
{
	unsigned char c;
	while (m_MidiFd >= 0 && read(m_MidiFd, &c, 1) == 1)
		if (m_Bytes.Push(c, packet)) return true;
	return false;
}

bool OssMidiTransport::Write(const MidiPacket &packet)
{
	if (m_MidiWrFd < 0) return false;
	const unsigned char message[3] = { packet.Status, packet.Data1, packet.Data2 };
	const unsigned size = MidiSize(packet.Status);
	return write(m_MidiWrFd, message, size) == (ssize_t)size;
}

std::string OssMidiTransport::Status() const
{
	if (!m_Error.empty()) return "OSS: " + m_Error;
	if (m_MidiFd < 0) return "OSS: closed";
	return "OSS: " + m_DeviceName;
}

// * Backend module entry

static void *Create(void *) { return static_cast<MidiBackend *>(new NativeMidiBackend(new OssMidiTransport)); }
static void Destroy(void *backend) { delete static_cast<MidiBackend *>(backend); }

extern "C" const BackendDescriptor *SpiralPlugin_GetMidiBackend()
{
	static const BackendDescriptor d = {SPIRAL_MIDI_PLUGIN_ABI, "midi", "oss", Create, Destroy};
	return &d;
}
