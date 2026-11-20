// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_VOICE_POOL_H
#define SPIRAL_VOICE_POOL_H

#include <map>
#include <string>
#include <vector>

#include "SpiralPlugin.h"

// Optional for devices with large saved state. Equal keys must describe the
// same settings for voice copies. The host still applies a full description
// when the key changes.
class VoiceStateKey
{
public:
	virtual ~VoiceStateKey() {}
	virtual void DescribeVoiceKey(Spiral::Core::Description &key) const = 0;
};

// A single device can supply per-voice CV without duplicating its external I/O.
// Other outputs, including the event stream, are shared by all consumers.
class VoiceOutputSource
{
public:
	virtual ~VoiceOutputSource() {}
	virtual Sample *VoiceOutput(unsigned port, unsigned voice) = 0;
};

class VoicePool
{
public:
	typedef SpiralPlugin *(*Lookup)(int id, void *host);
	struct Wire
	{
		int OutputID, OutputPort, InputID, InputPort;
	};

	VoicePool();
	~VoicePool();
	void SetLookup(Lookup find, void *host);

	void Add(int id, SpiralPlugin *device, const Spiral::Core::DeviceClass &kind, unsigned voices,
		const HostInfo *info, const std::string &sidecars);
	bool Holds(int id) const { return m_Members.count(id) != 0; }

	void SumOut(const Wire &wire, int size);
	void Feed(const Wire &wire, bool sourceHere);

	void Run(int id, SpiralPlugin *device, bool copies, bool reset, unsigned long frame);

	bool Follow();

	void Clear();
	void Clear(bool keepVoiceOutputs);

private:
	struct Member
	{
		std::vector<SpiralPlugin *> Copies;
		std::string State, Key;
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
