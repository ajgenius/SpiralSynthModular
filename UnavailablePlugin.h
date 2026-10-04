#ifndef UNAVAILABLE_PLUGIN_H
#define UNAVAILABLE_PLUGIN_H

#include "SpiralPlugin.h"
#include <string>

// Stand-in for a plugin id the host could not create. The state the host
// found for it under the file contract is kept as it was and given back
// on save. Ports are whatever the known layout and the wires need.
class UnavailablePlugin : public SpiralPlugin
{
public:
	explicit UnavailablePlugin(int pluginId);
	virtual void Execute();
	virtual void Describe(spiralcore::Description &d);
	virtual void Apply(spiralcore::Description::Reader &r);

	void SetSavedName(const std::string &name) { m_SavedName = name; }
	std::string SavedName() const;
	int InputCount() const { return m_PluginInfo.NumInputs; }
	int OutputCount() const { return m_PluginInfo.NumOutputs; }
	void EnsurePorts(int inputs, int outputs);

private:
	void ApplyKnownLayout();

	int m_PluginId;
	spiralcore::Description m_State;
	std::string m_SavedName;
};

#endif
