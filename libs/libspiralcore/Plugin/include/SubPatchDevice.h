// SPDX-License-Identifier: GPL-2.0-or-later
// The device a subpatch is hosted in, as the private tree's SubPatchDevice:
// built in, its settings its properties (Mode, Link, Buffer Frames, Link Name,
// Voices, Poly Output) and its panel a document bound to them. The file keeps
// them in the subpatch's own object, as the private tree writes it. It holds
// the ports on its edge, each bound through to the inner device port it
// stands for, and the graph of what it holds, which runs as its one step.
#ifndef SUBPATCH_DEVICE_H
#define SUBPATCH_DEVICE_H

#include "Device.h"
#include "GraphSort.h"
#include "PatchDescription.h"

// A port on a subpatch's edge: its name and type, as the file keeps them,
// and the inner device port it binds through to (-1 when unbound).
class BoundaryPort
{
public:
	BoundaryPort(): InnerID(-1), InnerPort(-1) {}
	BoundaryPort(int id, int port, const std::string &name, const std::string &type = std::string()):
		InnerID(id), InnerPort(port), Name(name), Type(type) {}

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
	// What it holds runs as its one step, last sorted first, as the host
	// runs a patch: each member takes the host's step for one device.
	virtual void Execute();
	// Its edge ports are its members', bound through: there is nothing of
	// its own to reset, only what it holds.
	virtual void Reset();
	// Its state is the subpatch's own object in the file, not a description.
	virtual void Describe(Spiral::Core::Description &d) {}
	virtual void Apply(Spiral::Core::Description::Reader &r) {}

	// The settings the subpatch runs with, on the GUI thread.
	const Spiral::Core::SubPatchConfig &Config() const { return m_Config; }
	// Runs with these, and sets its properties to them.
	void SetConfig(const Spiral::Core::SubPatchConfig &config);
	// Takes what its properties were set to since the last look; true when
	// that changed what runs. Isolated is not here yet, so a change to it is
	// put back.
	bool TakeSettings();

	// The ports on its edge, inputs and outputs in their order. After a
	// change, PortsChanged() makes them its plugin ports.
	std::vector<BoundaryPort> Inputs, Outputs;
	void PortsChanged();

	// The graph of what it holds, when it owns one; the canvas sorts it
	// afresh, under the audio cycle lock, whenever the patch changes.
	GraphSort &Graph() { return m_Graph; }
	void ClearGraph() { m_Graph = GraphSort(); }
	typedef void (*MemberStep)(int id, void *host);
	void SetMemberStep(MemberStep step, void *host) { m_Step = step; m_Host = host; }

private:
	void RunMembers();

	GraphSort m_Graph;
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
