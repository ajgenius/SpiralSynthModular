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


		/* Setup/Shutdown */
		virtual bool Setup();		/* Create Ports, Setup	*/
		virtual void Shutdown();	/* Disconnect, Cleanup	*/


		/* Port Management */
		virtual bool CreatePorts()=0;	/* Create All Ports	*/
		virtual void Clear();		/* Delete All Ports	*/
		virtual void Disconnect();	/* Disconnect All Ports */
		virtual void Reset();		/* Reset All Ports	*/


		/* Process */
		virtual void Process(unsigned int SampleCount)=0;


		/* Current Patch Settings */
		inline unsigned int SampleRate() { return m_SampleRate; }
		inline unsigned int SampleCount() { return m_SampleCount; }
		inline unsigned int VoiceCount() { return m_VoiceCount; }


		/* Permanent Device Settings */
		virtual bool IsMonophonic()=0;

		virtual bool SupportsInputs()=0;
		virtual bool SupportsOutputs()=0;
		virtual bool SupportsShadows()=0;

		virtual bool IsTerminal() { return false; }

		inline Patch *Host() { return m_Host; }

		/* Properties */
		inline bool IsLive() { return m_Initialized; }
		inline void SetVoice(unsigned int voice) { m_CurrentVoice = voice; }
		inline unsigned int GetVoice() { return m_CurrentVoice; }

		virtual InputPort* GetInputPort(unsigned int PortNumber);
		virtual OutputPort* GetOutputPort(unsigned int PortNumber);

		virtual ShadowPort* GetShadowPort(unsigned int PortNumber);


		vector<ShadowPort*> ShadowPorts() { return m_ShadowPorts; }

		/* Convenience Macros For Common DSP Needs */
		#define InputExists(input_port) input_port->IsConnected()
		#define GetInput(input_port, pos) input_port->GetSampleValue(pos, GetVoice())
		#define GetInputPitch(input_port, pos) input_port->GetSamplePitch(pos, GetVoice())

		#define OutputExists(output_port) output_port->IsConnected()
		#define SetOutput(output_port, pos, value) output_port->SetSampleValue(pos, value, GetVoice())
		#define SetOutputPitch(output_port, pos, value) output_port->SetSamplePitch(pos, value, GetVoice())

	protected:
		friend class InputPort;
		vector<InputPort*> m_InputPorts;

		friend class ShadowPort;
		vector<ShadowPort*> m_ShadowPorts;

		friend class OutputPort;
		vector<OutputPort*> m_OutputPorts;

		map<string,Property*> m_SharedProperties;

		#define ValueAsInt(property) property.Value.AsSigned
		#define ValueAsUInt(property) property.Value.AsUnsigned
		#define ValueAsFloat(property) property.Value.AsFloat
		#define ValueSetInt(property, new) property.Value.AsSigned = new
		#define ValueSetUInt(property, new) property.Value.AsUnsigned = new
		#define ValueSetFloat(property, new) property.Value.AsFloat = new
		#define ValueAsBoolean(property) property.Value.AsBoolean
		#define ValueSetBoolean(property, new) property.Value.AsBoolean = new


		inline void RegisterSharedProperty(Property *Property, string Label, string Hint)
		{
			assert(m_SharedProperties[Label]==NULL);

			Property->SetLabel(Label);
			Property->SetHint(Hint);

			m_SharedProperties[Label] = Property;
		}

		typedef vector<NumericPropertyValue> StateProperties;
		StateProperties m_DefaultState;
		vector<StateProperties> m_StateValues;
		
		inline unsigned int NewStateProperty(bool Initializer)
		{
			NumericPropertyValue init = DefaultBoolean(Initializer);
			m_DefaultState.push_back(init);

			for (unsigned int i=0; i<m_StateValues.size(); i++)
			{
				m_StateValues[i].push_back(init);
			}

			return m_DefaultState.size() - 1;
		}

		inline unsigned int NewStateProperty(int64_t Initializer)
		{
			NumericPropertyValue init = DefaultSigned(Initializer);
			m_DefaultState.push_back(init);

			for (unsigned int i=0; i<m_StateValues.size(); i++)
			{
				m_StateValues[i].push_back(init);
			}

			return m_DefaultState.size() - 1;
		}

		inline unsigned int NewStateProperty(uint64_t Initializer)
		{
			NumericPropertyValue init = DefaultUnsigned(Initializer);
			m_DefaultState.push_back(init);

			for (unsigned int i=0; i<m_StateValues.size(); i++)
			{
				m_StateValues[i].push_back(init);
			}

			return m_DefaultState.size() - 1;
		}

		inline unsigned int NewStateProperty(double Initializer)
		{
			NumericPropertyValue init = DefaultFloat(Initializer);
			m_DefaultState.push_back(init);

			for (unsigned int i=0; i<m_StateValues.size(); i++)
			{
				m_StateValues[i].push_back(init);
			}

			return m_DefaultState.size() - 1;
		}

		inline NumericPropertyValue *StateValue(unsigned int index) 
		{ 
			return &m_StateValues[m_CurrentVoice][index]; 
		}
	private:
		unsigned int m_SampleRate, m_SampleCount, m_VoiceCount, m_CurrentVoice;

		bool m_Initialized;

		Patch *m_Host;
	};
}

#endif /* __DEVICE_H__ */
