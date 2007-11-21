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

// Device is the basic top level host for ports
// And the root ancestor for DSP's

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <list>
#include <string>
#include <algorithm>
#include <cctype>

#include "Property.h"

using namespace std;

namespace Spiral
{
	class Patch;

	class InputPort;
	class OutputPort;
	class ShadowPort;

	class Device
	{
	public:
		Device(Patch *Host);
		virtual ~Device();

		/* Setting Properties */
		virtual bool LoadProperties(map<string,Property*> &properties);
		bool CopyDevice(Device *device);
		virtual bool UpdateProperty(string name) { return true; }

		/* Setup/Shutdown */
		virtual bool Setup();		/* Create Ports, Setup	*/
		virtual void Shutdown();	/* Disconnect, Cleanup	*/


		/* Port Management */
		virtual bool CreatePorts()=0;	/* Create All Ports	*/
		virtual void Clear();		/* Delete All Ports	*/
		virtual void Disconnect();	/* Disconnect All Ports */
		virtual void Reset();		/* Reset All Ports	*/


		/* Process */
		virtual void Process(UnsignedType SampleCount)=0;

		/* Current Patch Settings */
		inline UnsignedType SampleRate() { return m_SampleRate; }
		inline UnsignedType SampleCount() { return m_SampleCount; }
		inline UnsignedType VoiceCount() { return m_VoiceCount; }


		/* Permanent Device Features */
		virtual bool IsMonophonic() { return false; }

		virtual bool SupportsInputs() { return true; }
		virtual bool SupportsOutputs() { return true; }
		virtual bool SupportsShadows() { return true; }

		virtual bool IsTerminal() { return false; }

		inline Patch *Host() { return m_Host; }

		inline bool IsLive() { return m_Initialized; }
		inline void SetVoice(UnsignedType voice) 
		{ 
			#ifdef BOUNDS_CHECK
			if (UNLIKELY(voice >= m_VoiceCount))
			{
				cerr << "Voice Index Out Of Bounds" << endl;
			}
			#endif

			m_CurrentVoice = voice; 
		}

		inline UnsignedType GetVoice() { return m_CurrentVoice; }

		/* Ports */
		virtual InputPort* GetInputPort(UnsignedType PortNumber);
		virtual OutputPort* GetOutputPort(UnsignedType PortNumber);

		virtual ShadowPort* GetShadowPort(UnsignedType PortNumber);

		inline vector<InputPort*> InputPorts() { return m_InputPorts; }
		inline vector<OutputPort*> OutputPorts() { return m_OutputPorts; }
		inline vector<ShadowPort*> ShadowPorts() { return m_ShadowPorts; }

		/* Convenience Macros For Common DSP Needs */
		#define InputExists(input_port) input_port->IsConnected()
		#define GetInputBuffer(input_port) input_port->GetSampleBuffer(GetVoice())
		#define GetInput(input_port, pos) input_port->GetSampleValue(pos, GetVoice())
		#define GetInputPitch(input_port, pos) input_port->GetSamplePitch(pos, GetVoice())

		#define OutputExists(output_port) output_port->IsConnected()
		#define GetOutputBuffer(output_port) output_port->GetSampleBuffer(GetVoice())
		#define SetOutput(output_port, pos, value) output_port->SetSampleValue(pos, value, GetVoice())
		#define SetOutputPitch(output_port, pos, value) output_port->SetSamplePitch(pos, value, GetVoice())

		/* Shared Properties */
		inline Property* DuplicateProperty(string &Key)
		{
			if (m_State.Properties.count(Key))
			{
				return m_State.Properties[Key]->Duplicate();
			}

			return NULL;
		}

		inline void DuplicateProperties(map<string,Property*> &External)
		{
			map<string, Property*>::iterator prop;

			for (prop = m_State.Properties.begin();
				prop != m_State.Properties.end();
				prop++)
			{
				if (!External.count(prop->first))
					External[prop->first] = prop->second->Duplicate();
			}
		}

		inline void SynchronizeProperty(string Key, Property *Value)
		{
			if (m_State.Properties.count(Key))
			{
				Property *shared = m_State.Properties[Key];

				if (shared->IsReadOnly())
					Value->CopyValue(*shared);
				else
				{
					shared->CopyValue(*Value);
	
					if (shared->RequiresUpdates())
						UpdateProperty(Key);
				}

				return;
			}

			//WARNING?
		}

	protected:
		/* Ports */
		friend class InputPort;
		vector<InputPort*> m_InputPorts;

		friend class ShadowPort;
		vector<ShadowPort*> m_ShadowPorts;

		friend class OutputPort;
		vector<OutputPort*> m_OutputPorts;

		/* Properties */
		typedef vector<NumericPropertyValue> StateProperties;
		
		/* Per Class Properties*/
		UnsignedProperty DeviceVersion;
		StringProperty DeviceName;

		/* State Properties */	
		struct
		{
			/* Per Instance Properties */
			map<string,Property*> Properties;
			map<Property*,Property*> UpdateData; /* For ProcessUpdate */

			/* Per Voice State Values */
			StateProperties DefaultValues;
			vector<StateProperties> Values;
		} m_State;


		#define ValueAsInt(property) property.Value.AsSigned
		#define ValueAsUInt(property) property.Value.AsUnsigned
		#define ValueAsFloat(property) property.Value.AsFloat
		#define ValueSetInt(property, new) property.Value.AsSigned = new
		#define ValueSetUInt(property, new) property.Value.AsUnsigned = new
		#define ValueSetFloat(property, new) property.Value.AsFloat = new
		#define ValueAsBoolean(property) property.Value.AsBoolean
		#define ValueSetBoolean(property, new) property.Value.AsBoolean = new

		#define AsSigned(property) property.Value.AsSigned
		#define AsUnsigned(property) property.Value.AsUnsigned
		#define AsFloat(property) property.Value.AsFloat
		#define SetSigned(property, new) property.Value.AsSigned = new
		#define SetUnsigned(property, new) property.Value.AsUnsigned = new
		#define SetFloat(property, new) property.Value.AsFloat = new

		#define AsBoolean(property) property.Value.AsBoolean
		#define SetBoolean(property, new) property.Value.AsBoolean = new

		#define AsString(property) property.Value
		#define SetString(property, new) property.Value = new

		/* Rethink all of this registering crap - simple is best */
		inline void UnregisterSharedProperty(string Label)
		{
			string key = Label;
			transform (key.begin(), key.end(), key.begin(), (int(*)(int))std::toupper);

			m_State.UpdateData.erase(m_State.Properties[key]);
			m_State.Properties.erase(key);
		}

		inline void RegisterSharedProperty(Property &Initializer, string Label, string Hint)
		{
			string key = Label;
			transform (key.begin(), key.end(), key.begin(), (int(*)(int))std::toupper);

			assert(m_State.Properties[key]==NULL);

			m_State.Properties[key] = &Initializer;
			m_State.Properties[key]->SetLabel(Label);
			m_State.Properties[key]->SetHint(Hint);
		}

		/* Voice State Properties */
		inline UnsignedType NewStateProperty(bool Initializer)
		{
			NumericPropertyValue init = DefaultBoolean(Initializer);
			m_State.DefaultValues.push_back(init);

			for (UnsignedType i=0; i<m_State.Values.size(); i++)
			{
				m_State.Values[i].push_back(init);
			}

			return m_State.DefaultValues.size() - 1;
		}

		inline UnsignedType NewStateProperty(SignedType Initializer)
		{
			NumericPropertyValue init = DefaultSigned(Initializer);
			m_State.DefaultValues.push_back(init);

			for (UnsignedType i=0; i<m_State.Values.size(); i++)
			{
				m_State.Values[i].push_back(init);
			}

			return m_State.DefaultValues.size() - 1;
		}

		inline UnsignedType NewStateProperty(UnsignedType Initializer)
		{
			NumericPropertyValue init = DefaultUnsigned(Initializer);
			m_State.DefaultValues.push_back(init);

			for (UnsignedType i=0; i<m_State.Values.size(); i++)
			{
				m_State.Values[i].push_back(init);
			}

			return m_State.DefaultValues.size() - 1;
		}

		inline UnsignedType NewStateProperty(double Initializer)
		{
			NumericPropertyValue init = DefaultFloat(Initializer);
			m_State.DefaultValues.push_back(init);

			for (UnsignedType i=0; i<m_State.Values.size(); i++)
			{
				m_State.Values[i].push_back(init);
			}

			return m_State.DefaultValues.size() - 1;
		}

		inline NumericPropertyValue *StateValue(UnsignedType index) 
		{ 
			return &m_State.Values[m_CurrentVoice][index]; 
		}
	private:
		UnsignedType m_SampleRate, m_SampleCount, m_VoiceCount, m_CurrentVoice;

		bool m_Initialized;

		Patch *m_Host;
	};
}

#endif /* __DEVICE_H__ */
