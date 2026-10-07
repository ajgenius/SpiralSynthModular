// SPDX-License-Identifier: GPL-2.0-or-later
// A FIFO stands in for /dev/midi: bytes written to it come out of Poll
// as packets, running status and a clock tick included; a Send lands in
// the FIFO the other way. No device needed.
#include "MidiBackend.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

extern "C" const spiralcore::BackendDescriptor *SpiralPlugin_GetMidiBackend();
using namespace spiralcore;

int main()
{
	char in[64], out[64];
	snprintf(in, sizeof(in), "/tmp/oss-midi-in-%ld", long(getpid()));
	snprintf(out, sizeof(out), "/tmp/oss-midi-out-%ld", long(getpid()));
	assert(!mkfifo(in, 0600) && !mkfifo(out, 0600));
	// Both ends open before the backend connects, so its opens never block.
	const int writer = open(in, O_RDWR), reader = open(out, O_RDWR | O_NONBLOCK);
	assert(writer >= 0 && reader >= 0);

	const BackendDescriptor *d = SpiralPlugin_GetMidiBackend();
	assert(d && std::string(d->Kind) == "midi" && std::string(d->Name) == "oss");
	MidiBackend *backend = static_cast<MidiBackend *>(d->Create(NULL));
	backend->Select(in, out);
	for (int n = 0; n < 300 && backend->Ports(false).empty(); ++n) usleep(10000);
	printf("%s\n", backend->Status().c_str());
	assert(backend->Ports(false).size() == 1 && backend->Ports(false)[0] == in);

	const unsigned char bytes[] = { 0x90, 60, 100, 0xf8, 64, 90, 0xc2, 5, 0x80, 60, 0 };
	assert(write(writer, bytes, sizeof(bytes)) == (ssize_t)sizeof(bytes));
	MidiPacket p;
	std::vector<MidiPacket> got;
	for (int n = 0; n < 300 && got.size() < 5; ++n)
	{
		while (backend->Poll(p))
			if (p.Status != 0xff) got.push_back(p);
		usleep(10000);
	}
	for (size_t n = 0; n < got.size(); ++n) printf("poll %zu: %02x %d %d\n", n, got[n].Status, got[n].Data1, got[n].Data2);
	assert(got.size() == 5);
	assert(got[0].Status == 0x90 && got[0].Data1 == 60 && got[0].Data2 == 100);
	assert(got[1].Status == 0xf8);
	assert(got[2].Status == 0x90 && got[2].Data1 == 64 && got[2].Data2 == 90);
	assert(got[3].Status == 0xc2 && got[3].Data1 == 5);
	assert(got[4].Status == 0x80 && got[4].Data1 == 60);

	assert(backend->Send(MidiPacket(0x91, 67, 77)));
	unsigned char sent[3] = { 0, 0, 0 };
	for (int n = 0; n < 300 && read(reader, sent, 3) != 3; ++n) usleep(10000);
	assert(sent[0] == 0x91 && sent[1] == 67 && sent[2] == 77);

	d->Destroy(backend);
	close(writer); close(reader);
	unlink(in); unlink(out);
	puts("OSS backend reads packets off the device and writes them back PASS");
	return 0;
}
