// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SUBPATCH_DEVICE_H
#define SUBPATCH_DEVICE_H

#include "Device.h"
#include "GraphSort.h"
#include "PatchDescription.h"
#include "VoicePool.h"

class BoundaryPort
{
public:
	BoundaryPort(): InnerID(-1), InnerPort(-1) {}
	BoundaryPort(int id, int port, const std::string &name, const std::string &type = std::string()):
		InnerID(id), InnerPort(port), Name(name), Type(type) {}

	bool IsEvent() const { return Type == "MusicalEventPort"; }

	int InnerID;
	int InnerPort;
	std::string Name;
	std::string Type;
};

class SubPatchDevice : public Spiral::Core::Device
{
public:
	static const Spiral::Core::DeviceClass &Class();

	SubPatchDevice();
	virtual void Execute();
	virtual void Reset();
	virtual void Describe(Spiral::Core::Description &d) {}
	virtual void Apply(Spiral::Core::Description::Reader &r) {}
	virtual bool GeneratePanel(const std::string &Directory, std::string &Document);

	const Spiral::Core::SubPatchConfig &Config() const { return m_Config; }
	void SetConfig(const Spiral::Core::SubPatchConfig &config);
	bool TakeSettings();

	Spiral::File::DocumentSection Document;
	std::vector<BoundaryPort> Inputs, Outputs;
	void PortsChanged();

	GraphSort &Graph() { return m_Graph; }
	void ClearGraph() { m_Graph = GraphSort(); }
	typedef void (*MemberStep)(int id, void *host);
	void SetMemberStep(MemberStep step, void *host) { m_Step = step; m_Host = host; }

	VoicePool &Voices() { return m_Pool; }

private:
	void RunMembers();

	GraphSort m_Graph;
	VoicePool m_Pool;
	MemberStep m_Step;
	void *m_Host;
	Spiral::Core::SubPatchConfig Settings();

	Spiral::Core::SubPatchConfig m_Config;
	Spiral::Core::SetProperty m_Mode, m_Link;
	Spiral::Core::UnsignedProperty m_BufferFrames, m_Voices;
	Spiral::Core::StringProperty m_LinkName;
	Spiral::Core::ToggleProperty m_PolyOutput;
};

#endif
