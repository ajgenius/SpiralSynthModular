// Copyright (C) 2007 ajgenius <113943+ajgenius@users.noreply.github.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// Device is the abstract top level host for ports

#include "Port.h"
#include "Device.h"
#include "Patch.h"

//#define DEBUG_LOADING
using namespace std;
using namespace Spiral;

Device::Device(Patch *Host)
{
	assert(Host!=NULL);

	m_Host = Host;

	Host->m_Devices.push_back(this);

	m_SampleRate = m_Host->SampleRate();
	m_SampleCount = m_Host->SampleCount();
	m_VoiceCount = m_Host->VoiceCount();
	m_CurrentVoice = 0;

	/* Used For Loading Information, and User Graph */
	DeviceVersion = UnsignedProperty(Property::ReadOnly, 1, 0, 0, 0, 0);
	DeviceName = StringProperty(Property::ReadOnly, "Device");

	m_Initialized = false;
}

Device::~Device()
{
	if (IsLive())
		Shutdown();

	Host()->m_Devices.erase(find(Host()->m_Devices.begin(), Host()->m_Devices.end(), this));

	Clear();
}

bool Device::LoadProperties(map<string,Property*> properties)
{
	/* See if this is a versioned set of properties */
	if (properties.count("VERSION"))
	{
		UnsignedProperty *version = dynamic_cast<UnsignedProperty*>(properties["VERSION"]);

		if (version->Value.AsUnsigned != DeviceVersion.Value.AsUnsigned)
		{
			cerr << "Version Mismatch (" << version->Value.AsUnsigned << " != " << DeviceVersion.Value.AsUnsigned 
				<<") in property loading! This probably indicates a bug in " << DeviceName.Value << endl;

			return false;
		}
	}
	
	map<string,Property*>::iterator iter;
	for (iter = m_State.Properties.begin(); iter != m_State.Properties.end(); iter++)
	{
		if (iter->second && iter->second->IsReadOnly())
			continue;


		#ifdef DEBUG_LOADING
		printf("Maybe Loading Property - %s\n", iter->first.c_str());
		#endif

		if (properties.count(iter->first))
		{
			#ifdef DEBUG_LOADING
			printf("Almost Certainly Loading Property - %s which is of type %s\n", iter->first.c_str(), iter->second->ClassName().c_str());
			#endif

			{
				iter->second->CopyValue(*properties[iter->first]);

				if (iter->second->RequiresUpdates())
				{
					UpdateProperty(iter->first);
				}
			}
		}
	}

	return true;
}

bool Device::CopyDevice(Device *device)
{
	if (!device)
	{
		// Gimme an error
		return false;
	}

	if (typeid(device) != typeid(this))
	{
		// Gimme an error
		return false;
	}

	return LoadProperties(device->m_State.Properties);
}

bool Device::Setup()
{
	m_Initialized = CreatePorts();

	if (m_Initialized)
		Reset();

	return m_Initialized;
}

void Device::Shutdown()
{
	Disconnect();

	/* Fo Safety Set After Wire Cleanup */
	m_Initialized = false;
}

void Device::Clear()
{
	while (!m_ShadowPorts.empty())
	{
		delete m_ShadowPorts.back();
	}

	while (!m_InputPorts.empty())
	{
		delete m_InputPorts.back();
	}

	while (!m_OutputPorts.empty())
	{
		delete m_OutputPorts.back();
	}
}

void Device::Disconnect()
{
	vector<InputPort*>::iterator in_port;

	for (in_port = m_InputPorts.begin(); 
		in_port!=m_InputPorts.end(); in_port++)
	{
		(*in_port)->Disconnect();
	}

	vector<OutputPort*>::iterator out_port;

	for (out_port = m_OutputPorts.begin(); 
		out_port!=m_OutputPorts.end(); out_port++)
	{
		(*out_port)->Disconnect();
	}
}

void Device::Reset()
{
	m_SampleRate = m_Host->SampleRate();
	m_SampleCount = m_Host->SampleCount();
	m_VoiceCount = m_Host->VoiceCount();
	m_CurrentVoice = 0;

	/* Only Directly Reset Output Ports */
	vector<OutputPort*>::iterator port;

	for (port = m_OutputPorts.begin(); 
		port!=m_OutputPorts.end(); port++)
	{
		(*port)->Reset();
	}

	while (m_VoiceCount > m_State.Values.size())
	{
		m_State.Values.push_back(m_State.DefaultValues);
	}

	while (m_VoiceCount < m_State.Values.size())
	{
		m_State.Values.pop_back();
	}
}

InputPort* Device::GetInputPort(UnsignedType PortNumber)
{
	if (PortNumber < m_InputPorts.size())
	{
		return m_InputPorts[PortNumber];
	}

	return NULL;
}

OutputPort* Device::GetOutputPort(UnsignedType PortNumber)
{
	if (PortNumber < m_OutputPorts.size())
	{
		return m_OutputPorts[PortNumber];
	}

	return NULL;
}

ShadowPort* Device::GetShadowPort(UnsignedType PortNumber)
{
	if (PortNumber < m_ShadowPorts.size())
	{
		return m_ShadowPorts[PortNumber];
	}

	return NULL;
}
