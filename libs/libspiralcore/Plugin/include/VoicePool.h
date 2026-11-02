// SPDX-License-Identifier: GPL-2.0-or-later
// A patch's voice pool. The first voice is each device itself; past it, the
// pool keeps a copy of the device per voice, and the sums of what the voices
// send out of the pool. The patch whose voice count it is owns it: a
// subpatch's device, or the host for the patch itself. The host says which
// devices and wires are in it; the pool makes, runs, updates and undoes the
// copies.
#ifndef SPIRAL_VOICE_POOL_H
#define SPIRAL_VOICE_POOL_H

#include <map>
#include <string>
#include <vector>

#include "SpiralPlugin.h"

class VoicePool
{
public:
	// The host's device by ID, or NULL once it has gone.
	typedef SpiralPlugin *(*Lookup)(int id, void *host);
	// A wire, by device and port, as the canvas keeps them.
	struct Wire
	{
		int OutputID, OutputPort, InputID, InputPort;
	};

	VoicePool();
	~VoicePool();
	void SetLookup(Lookup find, void *host);

	// The device joins the pool: voices - 1 copies of it, made by its class
	// from what it describes, each told its voice.
	void Add(int id, SpiralPlugin *device, const Spiral::Core::DeviceClass &kind, unsigned voices,
		const HostInfo *info, const std::string &sidecars);
	bool Holds(int id) const { return m_Members.count(id) != 0; }

	// A wire from one of its devices out of the pool: the device it reaches
	// reads the sum of the voices instead, until Clear().
	void SumOut(const Wire &wire, int size);
	// A wire into one of its devices: each copy reads the same voice of a
	// source in this pool, or else what the device itself reads.
	void Feed(const Wire &wire, bool sourceHere);

	// After the device itself has run or reset: its copies do, unless
	// copies is false, then the sums of its voices are made.
	void Run(int id, SpiralPlugin *device, bool copies, bool reset, unsigned long frame);

	// The copies take a change to their device's state; false when one did
	// not take it, so the pool must be built again.
	bool Follow();

	// Inputs read from the sums are given back their own sources, devices
	// are told they play alone again, and the copies and sums go.
	void Clear();

private:
	struct Member
	{
		std::vector<SpiralPlugin *> Copies;
		std::string State;
	};
	struct Sum
	{
		int Port;
		Sample *Buffer;
	};

	SpiralPlugin *Find(int id) const { return m_Find ? m_Find(id, m_Host) : 0; }

	std::map<int, Member> m_Members;
	std::map<int, std::vector<Sum> > m_Sums;
	std::vector<Wire> m_Restore;
	Lookup m_Find;
	void *m_Host;
};

#endif
