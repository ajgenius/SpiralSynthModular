/*  LADSPAPlugin.h
 *  Copyleft (C) 2001 David Griffiths <dave@pawfal.org>
 *  LADSPA Plugin by Nicolas Noble <nicolas@nobis-crew.org>
 *  Modified by Mike Rawes <myk@waxfrenzy.org>
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

#ifndef __ladspa_plugin_h__
#define __ladspa_plugin_h__

#include <config.h>

#include <ladspa.h>

#include "Device.h"
#include "LADSPAInfo.h"

struct PortSetting
{
	float Min;
	float Max;
	bool  Clamp;
	float LogBase;     // >1.0 -> Logarithmic, otherwise linear
	bool  Integer;
};
struct PortValue
{
	float Value;
	bool  Connected;
};

template <class PropertyClass, class ValueType, ValueType Spiral::Core::NumericPropertyValue::*Member>
class PortSettings
{
public:
	Spiral::Core::VectorProperty List;

	size_t size() const { return const_cast<Spiral::Core::VectorProperty &>(List).size(); }
	ValueType &operator[](size_t n) { return ((PropertyClass *)List[n])->Value.*Member; }
	PropertyClass &At(size_t n) { return *(PropertyClass *)List[n]; }
	void push_back(ValueType v) { List.Add(Make(v)); }
	void clear() { while (List.size()) List.RemoveLast(); }
	void resize(size_t n) { while (List.size() > n) List.RemoveLast(); while (List.size() < n) push_back(ValueType()); }
private:
	static PropertyClass *Make(ValueType v);
};

template <> inline Spiral::Core::FloatProperty *
PortSettings<Spiral::Core::FloatProperty, Spiral::Core::FloatType, &Spiral::Core::NumericPropertyValue::AsFloat>::Make(Spiral::Core::FloatType v)
{ return new Spiral::Core::FloatProperty(DefaultLinearFlags, v, -0x7FFFFF, 0x7FFFFF, 0.0001f, 0.01f); }

template <> inline Spiral::Core::ToggleProperty *
PortSettings<Spiral::Core::ToggleProperty, bool, &Spiral::Core::NumericPropertyValue::AsBoolean>::Make(bool v)
{ return new Spiral::Core::ToggleProperty(Spiral::Core::Property::WriteOnly, v); }

typedef PortSettings<Spiral::Core::FloatProperty, Spiral::Core::FloatType, &Spiral::Core::NumericPropertyValue::AsFloat> PortFloats;
typedef PortSettings<Spiral::Core::ToggleProperty, bool, &Spiral::Core::NumericPropertyValue::AsBoolean> PortFlags;

class LADSPAPlugin : public Spiral::Core::Device
{
public:
	static const Spiral::Core::DeviceClass &Class();

	LADSPAPlugin();
	virtual ~LADSPAPlugin();

	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void Reset();
	virtual void ExecuteCommands();
	virtual void Describe(Spiral::Core::Description &d);
	virtual void Apply(Spiral::Core::Description::Reader &r);

	unsigned long  GetUniqueID() { return m_UniqueID; }
	bool           HasEffect() { return m_PlugDesc != NULL; }
	virtual void RestoreUnavailablePorts(int inputs, int outputs);
	const char    *GetName() { return (const char *)m_Name; }
	virtual const std::vector<LADSPAInfo::PluginEntry> &GetMenuList() const;
	const char    *GetMaker() { return (const char *)m_Maker; }
	int            GetPage() { return AsSigned(m_Page); }
	bool           GetUpdateInputs() { return AsBoolean(m_UpdateInputs); }
	unsigned long  GetInputPortCount() { return m_InputPortCount; }
	unsigned long  GetUnconnectedInputs() { return AsUnsigned(m_UnconnectedInputs); }
	const char    *GetInputPortName(unsigned long p)
	{
		if (!m_OutData.InputPortNames || p >= m_MaxInputPortCount)
			return "";
		return (const char *)(m_OutData.InputPortNames + p * 256);
	}
	PortSetting GetInputPortSetting(unsigned long p)
	{
		PortSetting empty = { -1.0f, 1.0f, true, 0.0f, false };
		if (!m_OutData.InputPortSettings || p >= m_MaxInputPortCount)
			return empty;
		return m_OutData.InputPortSettings[p];
	}
	float          GetInputPortDefault(unsigned long p)
	{
		if (!m_OutData.InputPortDefaults || p >= m_MaxInputPortCount)
			return 0.0f;
		return m_OutData.InputPortDefaults[p];
	}
	PortValue GetInputPortValue(unsigned long p)
	{
		PortValue empty = { 0.0f, false };
		if (!m_OutData.InputPortValues || p >= m_MaxInputPortCount)
			return empty;
		return m_OutData.InputPortValues[p];
	}

	enum GUICommands
	{
		NONE,
		SETPAGE,
		SELECTPLUGIN,
		CLEARPLUGIN,
		SETUPDATEINPUTS,
		SETDEFAULT,
		SETMIN,
		SETMAX,
		SETCLAMP,
		SETPORTENABLED
	};

	virtual bool     GeneratePanel(const std::string &Directory, std::string &Document);
	virtual unsigned PanelGeneration();

private:
	bool UpdatePlugin(unsigned long UniqueID);
	bool SelectPlugin(unsigned long UniqueID);
	void ClearPlugin(void);
	void ResetPortSettings(void);
	void SyncPortEnabled(void);
	void SetGUIExports(void);
	void SyncPortTypes(void);

	bool m_SSMPluginReset;

	const LADSPA_Descriptor     *m_PlugDesc;
	std::vector<LADSPA_Data*>    m_LADSPABufVec;
	LADSPA_Handle                m_PlugInstHandle;

	std::vector<int>        m_PortID;
	PortFloats              m_InputPortMin;
	PortFloats              m_InputPortMax;
	PortFlags               m_InputPortClamp;
	PortFlags               m_InputPortEnabled;
	PortFloats              m_InputPortDefault;

	int             m_Version;

	static LADSPAInfo          *m_LADSPAInfo;
	static int                  InstanceCount;

	unsigned long   m_PluginIndex;
	unsigned long   m_UniqueID;
	Spiral::Core::UnsignedProperty m_UniqueIDShown;
	Spiral::Core::SignedProperty   m_Page;
	// Which panel is on show: the generic pages, or the one written for
	// the plugin loaded, when there is one. Not saved: a device from a
	// patch opens on the plugin's own, one newly added on the generic.
	Spiral::Core::SignedProperty   m_PanelMode;
	Spiral::Core::ToggleProperty   m_UpdateInputs;

	unsigned long   m_MaxInputPortCount;
	unsigned long   m_InputPortCount;
	char            m_Name[256];
	char            m_Maker[256];

	Spiral::Core::UnsignedProperty m_UnconnectedInputs;

	struct OutputChannelData
	{
		char         *InputPortNames;
		PortSetting  *InputPortSettings;
		PortValue    *InputPortValues;
		float        *InputPortDefaults;
	};

	struct InputChannelData
	{
		unsigned long UniqueID;
		int           Page;
		bool          UpdateInputs;
		unsigned long InputPortIndex;
		float         InputPortDefault;
		float         InputPortMin;
		float         InputPortMax;
		bool          InputPortClamp;
		bool          InputPortEnabled;
	};

	OutputChannelData     m_OutData;
	struct PanelChannelData
	{
		float        *Mins;
		float        *Maxs;
		bool         *Clamps;
		float        *Values;
		bool         *Connected;
		bool         *Enabled;
		int           Page;
		int           Mode;
		unsigned      Generation;
	};
	PanelChannelData      m_PanelData;
	void RefreshPanelData();
	void UsePorts();
	void PanelChanged() { m_PanelData.Generation++; }
	InputChannelData      m_InData;
};

#endif // __ladspa_plugin_h__
