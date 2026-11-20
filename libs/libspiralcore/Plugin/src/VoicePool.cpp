// SPDX-License-Identifier: GPL-2.0-or-later
#include "VoicePool.h"
#include "DeviceClassRegistry.h"
#include "SpiralPlugin.h"
#include "Sample.h"

#include <sstream>

static std::string ReadVoiceKey(const VoiceStateKey &source)
{
	Spiral::Core::Description key;
	source.DescribeVoiceKey(key);
	std::ostringstream text;
	key.Write(text);
	return text.str();
}

VoicePool::VoicePool(): m_Find(0), m_Host(0)
{
}

VoicePool::~VoicePool()
{
	Clear();
}

void VoicePool::SetLookup(Lookup find, void *host)
{
	m_Find = find;
	m_Host = host;
}

void VoicePool::Add(int id, SpiralPlugin *device, const Spiral::Core::DeviceClass &kind, unsigned voices,
	const HostInfo *info, const std::string &sidecars)
{
	Spiral::Core::Description state;
	device->Describe(state);
	std::vector<SpiralPlugin *> &copies = m_Members[id].Copies;
	for (unsigned v = 1; v < voices; ++v)
	{
		SpiralPlugin *copy = kind.CreateInstance();
		if (!copy)
			break;
		copy->Initialise(info);
		Spiral::Core::Description::Reader values(state);
		copy->Apply(values);
		copies.push_back(copy);
	}
	device->SetVoice(device, 0, copies.size() + 1);
	for (size_t v = 0; v < copies.size(); ++v)
	{
		copies[v]->SetVoice(device, v + 1, copies.size() + 1);
		copies[v]->LoadExternalFiles(sidecars, id);
	}
	std::ostringstream text;
	state.Write(text);
	m_Members[id].State = text.str();
	if (const VoiceStateKey *source = dynamic_cast<const VoiceStateKey *>(device))
		m_Members[id].Key = ReadVoiceKey(*source);
}

void VoicePool::SumOut(const Wire &wire, int size)
{
	std::vector<Sum> &sums = m_Sums[wire.OutputID];
	Sample *sum = 0;
	for (size_t s = 0; s < sums.size() && !sum; ++s)
		if (sums[s].Port == wire.OutputPort)
			sum = sums[s].Buffer;
	if (!sum)
	{
		Sum added = {wire.OutputPort, new Sample(size)};
		sums.push_back(added);
		sum = added.Buffer;
	}
	if (SpiralPlugin *dest = Find(wire.InputID))
		dest->SetInput(wire.InputPort, sum);
	m_Restore.push_back(wire);
}

void VoicePool::Feed(const Wire &wire, bool sourceHere)
{
	std::map<int, Member>::iterator into = m_Members.find(wire.InputID);
	SpiralPlugin *visible = Find(wire.InputID);
	if (into == m_Members.end() || !visible)
		return;
	std::vector<SpiralPlugin *> &copies = into->second.Copies;
	std::map<int, Member>::iterator from = sourceHere ? m_Members.find(wire.OutputID) : m_Members.end();
	const std::vector<SpiralPlugin *> *sources = from == m_Members.end() ? 0 : &from->second.Copies;
	if (visible->IsEventPort(true, wire.InputPort))
	{
		for (size_t v = 0; v < copies.size(); ++v)
			copies[v]->SetEventInput(wire.InputPort, sources && v < sources->size()
				? (*sources)[v]->GetEventOutput(wire.OutputPort)
				: visible->GetEventInput(wire.InputPort));
		return;
	}
	for (size_t v = 0; v < copies.size(); ++v)
	{
		const Sample *in = visible->GetInput(wire.InputPort);
		Sample *out = 0;
		if (sources && v < sources->size() && (*sources)[v]->GetOutput(wire.OutputPort, &out))
			in = out;
		copies[v]->SetInput(wire.InputPort, in);
	}
}

void VoicePool::Run(int id, SpiralPlugin *device, bool copies, bool reset, unsigned long frame)
{
	std::map<int, Member>::iterator member = copies ? m_Members.find(id) : m_Members.end();
	std::vector<SpiralPlugin *> *voices = member == m_Members.end() ? 0 : &member->second.Copies;
	for (size_t v = 0; voices && v < voices->size(); ++v)
	{
		if (reset)
			(*voices)[v]->Reset();
		else
		{
			(*voices)[v]->Execute();
			(*voices)[v]->StampOutputs(frame);
		}
	}

	std::map<int, std::vector<Sum> >::iterator sums = m_Sums.find(id);
	for (size_t s = 0; !reset && sums != m_Sums.end() && s < sums->second.size(); ++s)
	{
		Sample *sum = sums->second[s].Buffer, *out = 0;
		if (!device->GetOutput(sums->second[s].Port, &out) || !out)
			continue;
		*sum = *out;
		for (size_t v = 0; voices && v < voices->size(); ++v)
			if ((*voices)[v]->GetOutput(sums->second[s].Port, &out) && out)
				for (int n = 0; n < sum->GetLength() && n < out->GetLength(); ++n)
					sum->Set(n, (*sum)[n] + (*out)[n]);
	}
}

bool VoicePool::Follow()
{
	for (std::map<int, Member>::iterator m = m_Members.begin(); m != m_Members.end(); ++m)
	{
		SpiralPlugin *device = Find(m->first);
		if (!device)
			continue;
		if (const VoiceStateKey *source = dynamic_cast<const VoiceStateKey *>(device))
		{
			std::string key = ReadVoiceKey(*source);
			if (key == m->second.Key)
				continue;
			m->second.Key.swap(key);
		}

		Spiral::Core::Description state;
		device->Describe(state);
		std::ostringstream text;
		state.Write(text);
		if (text.str() == m->second.State)
			continue;
		m->second.State = text.str();
		for (size_t v = 0; v < m->second.Copies.size(); ++v)
		{
			Spiral::Core::Description::Reader values(state);
			m->second.Copies[v]->Apply(values);
			Spiral::Core::Description applied;
			m->second.Copies[v]->Describe(applied);
			std::ostringstream copied;
			applied.Write(copied);
			if (copied.str() != text.str())
				return false;
		}
	}
	return true;
}

void VoicePool::Clear()
{
	for (size_t w = 0; w < m_Restore.size(); ++w)
	{
		SpiralPlugin *source = Find(m_Restore[w].OutputID);
		SpiralPlugin *dest = Find(m_Restore[w].InputID);
		Sample *out = 0;
		if (source && dest && source->GetOutput(m_Restore[w].OutputPort, &out))
			dest->SetInput(m_Restore[w].InputPort, out);
	}
	m_Restore.clear();
	for (std::map<int, Member>::iterator m = m_Members.begin(); m != m_Members.end(); ++m)
	{
		if (SpiralPlugin *device = Find(m->first))
			device->SetVoice(device, 0, 1);
		for (size_t v = 0; v < m->second.Copies.size(); ++v)
			delete m->second.Copies[v];
	}
	m_Members.clear();
	for (std::map<int, std::vector<Sum> >::iterator s = m_Sums.begin(); s != m_Sums.end(); ++s)
		for (size_t n = 0; n < s->second.size(); ++n)
			delete s->second[n].Buffer;
	m_Sums.clear();
}
