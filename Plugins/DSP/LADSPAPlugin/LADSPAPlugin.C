/*  LADSPAPlugin.C
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
#include <config.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <iostream>

#include "SpiralIcon.xpm"
#include "LADSPAPlugin.h"
#include "LADSPAInfo.h"

using namespace std;

LADSPAInfo * LADSPAPlugin::m_LADSPAInfo= NULL;
int LADSPAPlugin::InstanceCount=0;

static bool ssm_finite(float f)
{
	return (f == f) && (f < 1e30f) && (f > -1e30f);
}

static float ssm_sanitize(float in, float replacement)
{
	if (!ssm_finite(in))
		return replacement;
	return in;
}

////////////////////////////////////////////////////

extern "C" {
SpiralPlugin* SpiralPlugin_CreateInstance()
{
	return new LADSPAPlugin;
}

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DSP;
}


const char** SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}

int SpiralPlugin_GetID()
{
	return 0x0016;
}

string SpiralPlugin_GetGroupName()
{
	return "Filters/FX";
}
}

///////////////////////////////////////////////////////

LADSPAPlugin::LADSPAPlugin()
{
	InstanceCount++;
	m_LADSPAInfo = LADSPAInfo::Get();

	m_PlugDesc = NULL;
	m_PlugInstHandle = NULL;
	m_SSMPluginReset = false;
	m_PluginIndex = 0;
	m_UniqueID = 0;
	m_Page = 2;
	m_UpdateInputs = true;
	m_InputPortCount = 0;
	m_UnconnectedInputs = 0;
	m_Version = 9;

	m_OutData.InputPortNames = NULL;
	m_OutData.InputPortSettings = NULL;
	m_OutData.InputPortValues = NULL;
	m_OutData.InputPortDefaults = NULL;

	memset(&m_InData, 0, sizeof(m_InData));
	strncpy(m_Name, "None", 5);
	strncpy(m_Maker, "None", 5);

	ClearPlugin();

	m_PluginInfo.Name="LADSPA";
	m_PluginInfo.Width=500;
	m_PluginInfo.Height=320;
	m_PluginInfo.NumInputs=0;
	m_PluginInfo.NumOutputs=0;

	m_MaxInputPortCount = m_LADSPAInfo->GetMaxInputPortCount();
	if (m_MaxInputPortCount < 1)
		m_MaxInputPortCount = 1;
	if (m_MaxInputPortCount > 4096)
		m_MaxInputPortCount = 4096;

	m_AudioCH->RegisterData("SetUniqueID", ChannelHandler::INPUT,&(m_InData.UniqueID), sizeof(m_InData.UniqueID));
	m_AudioCH->RegisterData("SetPage", ChannelHandler::INPUT,&(m_InData.Page), sizeof(m_InData.Page));
	m_AudioCH->RegisterData("SetUpdateInputs", ChannelHandler::INPUT,&(m_InData.UpdateInputs),sizeof(m_InData.UpdateInputs));
	m_AudioCH->RegisterData("SetInputPortIndex", ChannelHandler::INPUT, &(m_InData.InputPortIndex), sizeof(m_InData.InputPortIndex));
	m_AudioCH->RegisterData("SetInputPortDefault", ChannelHandler::INPUT, &(m_InData.InputPortDefault), sizeof(m_InData.InputPortDefault));
	m_AudioCH->RegisterData("SetInputPortMin", ChannelHandler::INPUT, &(m_InData.InputPortMin), sizeof(m_InData.InputPortMin));
	m_AudioCH->RegisterData("SetInputPortMax", ChannelHandler::INPUT, &(m_InData.InputPortMax), sizeof(m_InData.InputPortMax));
	m_AudioCH->RegisterData("SetInputPortClamp", ChannelHandler::INPUT, &(m_InData.InputPortClamp), sizeof(m_InData.InputPortClamp));

	m_AudioCH->RegisterData("GetName",ChannelHandler::OUTPUT,m_Name,256);
	m_AudioCH->RegisterData("GetMaker",ChannelHandler::OUTPUT,m_Maker,256);
	m_AudioCH->RegisterData("GetMaxInputPortCount",ChannelHandler::OUTPUT,&(m_MaxInputPortCount),sizeof(m_MaxInputPortCount));
	m_AudioCH->RegisterData("GetInputPortCount",ChannelHandler::OUTPUT,&(m_InputPortCount),sizeof(m_InputPortCount));

	m_OutData.InputPortNames = (char *)malloc(256 * m_MaxInputPortCount);
	m_OutData.InputPortSettings = (PortSetting *)malloc(sizeof(PortSetting) * m_MaxInputPortCount);
	m_OutData.InputPortValues = (PortValue *)calloc(m_MaxInputPortCount, sizeof(PortValue));
	m_OutData.InputPortDefaults = (float *)calloc(m_MaxInputPortCount, sizeof(float));

	if (m_OutData.InputPortNames &&
	    m_OutData.InputPortSettings &&
	    m_OutData.InputPortValues &&
	    m_OutData.InputPortDefaults)
	{
		memset(m_OutData.InputPortNames, 0, 256 * m_MaxInputPortCount);
		memset(m_OutData.InputPortSettings, 0, sizeof(PortSetting) * m_MaxInputPortCount);
		m_AudioCH->RegisterData("GetInputPortNames", ChannelHandler::OUTPUT, m_OutData.InputPortNames, 256 * m_MaxInputPortCount);
		m_AudioCH->RegisterData("GetInputPortSettings", ChannelHandler::OUTPUT, m_OutData.InputPortSettings, sizeof(PortSetting) * m_MaxInputPortCount);
		m_AudioCH->RegisterData("GetInputPortValues", ChannelHandler::OUTPUT, m_OutData.InputPortValues, sizeof(PortValue) * m_MaxInputPortCount);
		m_AudioCH->RegisterData("GetInputPortDefaults", ChannelHandler::OUTPUT, m_OutData.InputPortDefaults, sizeof(float) * m_MaxInputPortCount);
	} else {
		cerr<<"LADSPA Plugin: Memory allocation error"<<endl;
	}
}

LADSPAPlugin::~LADSPAPlugin()
{
	ClearPlugin();

	if (m_OutData.InputPortNames)  free(m_OutData.InputPortNames);
	if (m_OutData.InputPortSettings) free(m_OutData.InputPortSettings);
	if (m_OutData.InputPortValues) free(m_OutData.InputPortValues);
	if (m_OutData.InputPortDefaults) free(m_OutData.InputPortDefaults);

	InstanceCount--;
	if (m_LADSPAInfo && InstanceCount<=0)
	{
		LADSPAInfo::PackUpAndGoHome();
		m_LADSPAInfo = NULL;
	}
}

PluginInfo &LADSPAPlugin::Initialise(const HostInfo *Host)
{
	PluginInfo& Info = SpiralPlugin::Initialise(Host);
	SyncPortTypes();
	return Info;
}



void LADSPAPlugin::Reset()
{
	ResetPorts();

	m_SSMPluginReset = true;
	UpdatePlugin(m_UniqueID);
	m_SSMPluginReset = false;
}

void LADSPAPlugin::Execute()
{
	if (!m_PlugDesc || !m_PlugInstHandle || !m_HostInfo)
		return;

	int nInputs = m_PluginInfo.NumInputs;
	int nOutputs = m_PluginInfo.NumOutputs;
	if ((int)m_LADSPABufVec.size() < nInputs + nOutputs)
		return;
	if ((int)m_InputPortClamp.size() < nInputs ||
	    (int)m_InputPortMin.size() < nInputs ||
	    (int)m_InputPortMax.size() < nInputs ||
	    (int)m_InputPortDefault.size() < nInputs)
		return;

	for (int n=0; n<nInputs; n++)
	{
		if (!m_LADSPABufVec[n])
			continue;

		if (GetInput(n))
		{
			if (m_InputPortClamp[n]) {
				float Offset=m_InputPortMin[n];
				float Scale=m_InputPortMax[n]-m_InputPortMin[n];

				for (int i=0; i<m_HostInfo->BUFSIZE; i++)
				{
					float in = ssm_sanitize(GetInput(n,i), 0.0f);
					m_LADSPABufVec[n][i]=Offset+(in*0.5f+0.5f)*Scale;
				}
			} else {
				for (int i=0; i<m_HostInfo->BUFSIZE; i++)
				{
					m_LADSPABufVec[n][i]=ssm_sanitize(GetInput(n,i), 0.0f);
				}
			}
			if (m_OutData.InputPortValues && n < (int)m_MaxInputPortCount) {
				m_OutData.InputPortValues[n].Connected = true;
			}
			m_InputPortDefault[n] = m_LADSPABufVec[n][0];
		}
		else
		{
			for (int i=0; i<m_HostInfo->BUFSIZE; i++) {
				m_LADSPABufVec[n][i]=m_InputPortDefault[n];
			}
			if (m_OutData.InputPortValues && n < (int)m_MaxInputPortCount) {
				if (m_OutData.InputPortValues[n].Connected) {
					m_OutData.InputPortValues[n].Connected = false;
					m_InputPortDefault[n] = m_OutData.InputPortValues[n].Value;
				}
			}
		}
		if (m_OutData.InputPortValues && n < (int)m_MaxInputPortCount) {
			m_OutData.InputPortValues[n].Value = m_LADSPABufVec[n][0];
		}
		if (m_OutData.InputPortDefaults && n < (int)m_MaxInputPortCount) {
			m_OutData.InputPortDefaults[n] = m_InputPortDefault[n];
		}
	}

	m_PlugDesc->run(m_PlugInstHandle,m_HostInfo->BUFSIZE);

	for (int n=0; n<nOutputs; n++)
	{
		if (!m_LADSPABufVec[n+nInputs])
			continue;
		for (int i=0; i<m_HostInfo->BUFSIZE; i++)
		{
			SetOutput(n,i,m_LADSPABufVec[n+nInputs][i]);
		}
	}
}

void LADSPAPlugin::ExecuteCommands()
{
	if (m_AudioCH->IsCommandWaiting())
	{
		switch(m_AudioCH->GetCommand())
		{
			case (SETPAGE):
			{
				m_Page = m_InData.Page;
			}
			break;
			case (SELECTPLUGIN):
			{
				UpdatePlugin(m_InData.UniqueID);
			}
			break;
			case (CLEARPLUGIN):
			{
				ClearPlugin();
				UpdatePluginInfoWithHost();
			}
			break;
			case (SETUPDATEINPUTS):
			{
				m_UpdateInputs = m_InData.UpdateInputs;
			}
			break;
			case (SETDEFAULT):
			{
				if (m_InData.InputPortIndex < m_InputPortDefault.size() &&
				    m_OutData.InputPortDefaults &&
				    m_InData.InputPortIndex < m_MaxInputPortCount) {
					m_InputPortDefault[m_InData.InputPortIndex] = m_InData.InputPortDefault;
					m_OutData.InputPortDefaults[m_InData.InputPortIndex] = m_InData.InputPortDefault;
				}
			}
			break;
			case (SETMIN):
			{
				if (m_InData.InputPortIndex < m_InputPortMin.size() &&
				    m_OutData.InputPortSettings &&
				    m_InData.InputPortIndex < m_MaxInputPortCount) {
					m_InputPortMin[m_InData.InputPortIndex] = m_InData.InputPortMin;
					m_OutData.InputPortSettings[m_InData.InputPortIndex].Min = m_InData.InputPortMin;
				}
			}
			break;
			case (SETMAX):
			{
				if (m_InData.InputPortIndex < m_InputPortMax.size() &&
				    m_OutData.InputPortSettings &&
				    m_InData.InputPortIndex < m_MaxInputPortCount) {
					m_InputPortMax[m_InData.InputPortIndex] = m_InData.InputPortMax;
					m_OutData.InputPortSettings[m_InData.InputPortIndex].Max = m_InData.InputPortMax;
				}
			}
			break;
			case (SETCLAMP):
			{
				if (m_InData.InputPortIndex < m_InputPortClamp.size() &&
				    m_OutData.InputPortSettings &&
				    m_InData.InputPortIndex < m_MaxInputPortCount) {
					m_InputPortClamp[m_InData.InputPortIndex] = m_InData.InputPortClamp;
					m_OutData.InputPortSettings[m_InData.InputPortIndex].Clamp = m_InData.InputPortClamp;
				}
			}
		}
	}

	bool has_connection = false;
	for (int p = 0; p < m_PluginInfo.NumInputs && !has_connection; p++)
	{
		if (GetInput(p)) has_connection = true;
	}
	if (!has_connection && m_OutData.InputPortValues)
	{
		for (int p = 0; p < m_PluginInfo.NumInputs; p++)
		{
			if (p < (int)m_MaxInputPortCount)
				m_OutData.InputPortValues[p].Connected = false;
		}
	}
}

void LADSPAPlugin::StreamOut(ostream &s)
{
	s<<m_Version<<" ";

	switch (m_Version)
	{
		case 9:
		{
			m_UnconnectedInputs = m_PluginInfo.NumInputs;
			if (m_OutData.InputPortValues) {
				for (int p = 0; p < m_PluginInfo.NumInputs; p++) {
					if (p < (int)m_MaxInputPortCount &&
					    m_OutData.InputPortValues[p].Connected)
						m_UnconnectedInputs--;
				}
			}

			s<<m_Page<<" ";
			s<<m_UpdateInputs<<" ";
			s<<m_UniqueID<<" ";
			s<<m_InputPortMin.size()<<" ";
			s<<m_UnconnectedInputs<<" ";
			assert(m_InputPortMin.size()==m_InputPortMax.size());
			assert(m_InputPortMin.size()==m_InputPortClamp.size());
			assert(m_InputPortMin.size()==m_InputPortDefault.size());
			for (vector<float>::iterator i=m_InputPortMin.begin();
			     i!=m_InputPortMin.end(); i++)
			{
				float f = ssm_finite(*i)?(*i):0.0f;
				s<< f <<" ";
			}
			for (vector<float>::iterator i=m_InputPortMax.begin();
			     i!=m_InputPortMax.end(); i++)
			{
				float f = ssm_finite(*i)?(*i):0.0f;
				s<< f <<" ";
			}
			for (vector<char>::iterator i=m_InputPortClamp.begin();
			     i!=m_InputPortClamp.end(); i++)
			{
				s<< (int)(*i) <<" ";
			}
			for (vector<float>::iterator i=m_InputPortDefault.begin();
			     i!=m_InputPortDefault.end(); i++)
			{
				float f = ssm_finite(*i)?(*i):0.0f;
				s<< f <<" ";
			}
		}
		break;
		default:
			break;
	}
}

void LADSPAPlugin::StreamIn(istream &s)
{
	int Version = 0;
	float Gain = 0.0f;
	unsigned long UniqueID = 0;
	int PortCount = 0;
	float Min, Max;
	bool Clamp;
	float Default;

	ClearPlugin();

	s >> Version;

	switch (Version)
	{
		case 9:
		{
			s >> m_Page;
			s >> m_UpdateInputs;
			s >> UniqueID;
			s >> PortCount;
			s >> m_UnconnectedInputs;

			for (int n=0; n<PortCount; n++)
			{
				s >> Min;
				m_InputPortMin.push_back(Min);
			}

			for (int n=0; n<PortCount; n++)
			{
				s >> Max;
				m_InputPortMax.push_back(Max);
			}
			for (int n=0; n<PortCount; n++)
			{
				s >> Clamp;
				m_InputPortClamp.push_back(Clamp);
			}
			for (int n=0; n<PortCount; n++)
			{
				s >> Default;
				m_InputPortDefault.push_back(Default);
			}
		}
		break;
		case 8:
		{
			s >> m_Page;
			s >> m_UpdateInputs;
			s >> UniqueID;
			s >> PortCount;

			for (int n=0; n<PortCount; n++)
			{
				s >> Min;
				m_InputPortMin.push_back(Min);
			}

			for (int n=0; n<PortCount; n++)
			{
				s >> Max;
				m_InputPortMax.push_back(Max);
			}
			for (int n=0; n<PortCount; n++)
			{
				s >> Clamp;
				m_InputPortClamp.push_back(Clamp);
			}
			for (int n=0; n<PortCount; n++)
			{
				s >> Default;
				m_InputPortDefault.push_back(Default);
			}
		}
		break;
		case 7:
		{
			s >> m_Page;
			s >> UniqueID;
			s >> PortCount;

			for (int n=0; n<PortCount; n++)
			{
				s >> Min;
				m_InputPortMin.push_back(Min);
			}

			for (int n=0; n<PortCount; n++)
			{
				s >> Max;
				m_InputPortMax.push_back(Max);
			}
			for (int n=0; n<PortCount; n++)
			{
				s >> Clamp;
				m_InputPortClamp.push_back(Clamp);
			}
			for (int n=0; n<PortCount; n++)
			{
				s >> Default;
				m_InputPortDefault.push_back(Default);
			}
		}
		break;
		case 6:
		{
			s >> UniqueID;
			s >> PortCount;

			for (int n=0; n<PortCount; n++)
			{
				s >> Min;
				m_InputPortMin.push_back(Min);
			}

			for (int n=0; n<PortCount; n++)
			{
				s >> Max;
				m_InputPortMax.push_back(Max);
			}
			for (int n=0; n<PortCount; n++)
			{
				s >> Clamp;
				m_InputPortClamp.push_back(Clamp);
			}
			for (int n=0; n<PortCount; n++)
			{
				s >> Default;
				m_InputPortDefault.push_back(Default);
			}
		}
		break;
		case 5:
		{
			s >> Gain;
			s >> UniqueID;
			s >> PortCount;

			for (int n=0; n<PortCount; n++)
			{
				s >> Min;
				m_InputPortMin.push_back(Min);
			}

			for (int n=0; n<PortCount; n++)
			{
				s >> Max;
				m_InputPortMax.push_back(Max);
			}
			for (int n=0; n<PortCount; n++)
			{
				s >> Clamp;
				m_InputPortClamp.push_back(Clamp);
			}
			for (int n=0; n<PortCount; n++)
			{
				s >> Default;
				m_InputPortDefault.push_back(Default);
			}
		}
		break;
		case 4:
		{
			s >> Gain;
			s >> UniqueID;
			s >> PortCount;

			for (int n=0; n<PortCount; n++)
			{
				s >> Min;
				m_InputPortMin.push_back(Min);
			}

			for (int n=0; n<PortCount; n++)
			{
				s >> Max;
				m_InputPortMax.push_back(Max);
			}
			for (int n=0; n<PortCount; n++)
			{
				s >> Clamp;
				m_InputPortClamp.push_back(Clamp);
			}
			for (int n=0; n<PortCount; n++)
			{
				m_InputPortDefault.push_back(0.0f);
			}
		}
		break;

		case 3:
		{
			string Filename,Label;

			s >> Gain;
			s >> Filename;
			s >> Label;
			s >> PortCount;

			for (int n=0; n<PortCount; n++)
			{
				s >> Min;
				m_InputPortMin.push_back(Min);
			}

			for (int n=0; n<PortCount; n++)
			{
				s >> Max;
				m_InputPortMax.push_back(Max);
			}
			for (int n=0; n<PortCount; n++)
			{
				s >> Clamp;
				m_InputPortClamp.push_back(Clamp);
			}

			for (int n=0; n<PortCount; n++)
			{
				m_InputPortDefault.push_back(0.0f);
			}

			if (Filename!="None")
			{
				UniqueID = m_LADSPAInfo->GetIDFromFilenameAndLabel(Filename, Label);
			}
		}
		break;

		case 2:
		{
			string Filename, Label;

			s >> Gain;
			s >> Filename;
			s >> Label;
			s >> PortCount;

			for (int n=0; n<PortCount; n++)
			{
				s >> Min;
				m_InputPortMin.push_back(Min);
			}

			for (int n=0; n<PortCount; n++)
			{
				s >> Max;
				m_InputPortMax.push_back(Max);
			}

			for (int n=0; n<PortCount; n++)
			{
				m_InputPortClamp.push_back(true);
			}

			for (int n=0; n<PortCount; n++)
			{
				m_InputPortDefault.push_back(0.0f);
			}

			if (Filename!="None")
			{
				UniqueID = m_LADSPAInfo->GetIDFromFilenameAndLabel(Filename, Label);
			}
		}
		break;

		case 1:
		{
			string Filename, Label;

			s >> Gain;
			s >> Filename;
			s >> Label;

			if (Filename!="None")
			{
				UniqueID = m_LADSPAInfo->GetIDFromFilenameAndLabel(Filename, Label);
			}
		}
		break;
	}

	if (Version == 1) {
		UpdatePlugin(UniqueID);
	} else {
		if (Version < 9 && m_Page == 1) m_Page = 2;

		if (SelectPlugin(UniqueID)) {
			if (Version < 9) m_UnconnectedInputs = m_InputPortCount;
			SetGUIExports();
		} else {
			m_UniqueID = UniqueID;
			if (UniqueID)
				cerr << "Error loading LADSPA Plugin " << UniqueID << ".\n";
			strncpy(m_Name, "Missing LADSPA plugin", 255);
			strncpy(m_Maker, "Install the plugin and reload the patch", 255);
			m_Name[255] = '\0';
			m_Maker[255] = '\0';
		}
	}
}

bool LADSPAPlugin::UpdatePlugin(unsigned long UniqueID)
{
	ClearPlugin();
	if (SelectPlugin(UniqueID)) {
		ResetPortSettings();
		SetGUIExports();
		return true;
	}

	m_SSMPluginReset = false;
	if (UniqueID)
	{
		m_UniqueID = UniqueID;
		cerr << "Error loading LADSPA Plugin " << UniqueID << ".\n";
	}

	return false;
}

bool LADSPAPlugin::SelectPlugin(unsigned long UniqueID)
{
	if (UniqueID == 0) return false;
	if (!m_HostInfo) return false;

	m_PlugDesc = m_LADSPAInfo->GetDescriptorByID(UniqueID);

	if (m_PlugDesc) {
		if (!(m_PlugInstHandle = m_PlugDesc->instantiate(m_PlugDesc, m_HostInfo->SAMPLERATE))) {
			cerr << "WARNING: Could not instantiate plugin " << UniqueID << endl;
			m_LADSPAInfo->DiscardDescriptorByID(UniqueID);
			m_PlugDesc = 0;
			return false;
		}

		if (!m_PlugDesc->PortDescriptors || !m_PlugDesc->connect_port || !m_PlugDesc->run) {
			cerr << "WARNING: Plugin " << UniqueID << " is missing required LADSPA functions" << endl;
			if (m_PlugDesc->cleanup) m_PlugDesc->cleanup(m_PlugInstHandle);
			m_LADSPAInfo->DiscardDescriptorByID(UniqueID);
			m_PlugDesc = 0;
			m_PlugInstHandle = 0;
			return false;
		}

		if (! m_SSMPluginReset)
			for (unsigned long i = 0; i < m_PlugDesc->PortCount; i++) {
				if (LADSPA_IS_PORT_INPUT(m_PlugDesc->PortDescriptors[i])) {
					m_PluginInfo.NumInputs++;
				} else if (LADSPA_IS_PORT_OUTPUT(m_PlugDesc->PortDescriptors[i])) {
					m_PluginInfo.NumOutputs++;
				}
			}

		unsigned long c=0;
		for (unsigned int n=0; n<m_PlugDesc->PortCount; n++)
		{
			if (LADSPA_IS_PORT_INPUT(m_PlugDesc->PortDescriptors[n]))
			{
				LADSPA_Data *NewPort = new LADSPA_Data[m_HostInfo->BUFSIZE];
				m_LADSPABufVec.push_back(NewPort);
				m_PlugDesc->connect_port(m_PlugInstHandle, n, m_LADSPABufVec[c]);
 				m_PortID.push_back(n);
				c++;
			}
		}

		for (unsigned int n=0; n<m_PlugDesc->PortCount; n++)
		{
			if (LADSPA_IS_PORT_OUTPUT(m_PlugDesc->PortDescriptors[n]))
			{
				LADSPA_Data *NewPort = new LADSPA_Data[m_HostInfo->BUFSIZE];
				m_LADSPABufVec.push_back(NewPort);
				m_PlugDesc->connect_port(m_PlugInstHandle, n, m_LADSPABufVec[c]);
				m_PortID.push_back(n);
				c++;
			}
		}

		if (m_PlugDesc->activate)
			m_PlugDesc->activate(m_PlugInstHandle);

		if (! m_SSMPluginReset)
		{
			for (int n=0; n<m_PluginInfo.NumInputs; n++) AddInput();
			for (int n=0; n<m_PluginInfo.NumOutputs; n++) AddOutput();
		}

		if (! m_SSMPluginReset)
		{
			string desc;
			c=0;

			for (unsigned int i = 0; i < m_PlugDesc->PortCount; i++)
			{
				if (LADSPA_IS_PORT_INPUT(m_PlugDesc->PortDescriptors[i]))
				{
					const char *pname = m_PlugDesc->PortNames && m_PlugDesc->PortNames[i]
						? m_PlugDesc->PortNames[i] : "in";
					desc = string(pname) +
						(LADSPA_IS_PORT_CONTROL(m_PlugDesc->PortDescriptors[i]) ? " (CV)" : " (AU)");
					m_PluginInfo.PortTips.push_back(desc.c_str());

					c++;
				}
			}

			for (unsigned int i = 0; i < m_PlugDesc->PortCount; i++)
			{
				if (LADSPA_IS_PORT_OUTPUT(m_PlugDesc->PortDescriptors[i])) {

					const char *pname = m_PlugDesc->PortNames && m_PlugDesc->PortNames[i]
						? m_PlugDesc->PortNames[i] : "out";
					desc = string(pname) +
						(LADSPA_IS_PORT_CONTROL(m_PlugDesc->PortDescriptors[i]) ? " (CV)" : " (AU)");

					m_PluginInfo.PortTips.push_back(desc.c_str());
				}
			}

			SyncPortTypes();
			UpdatePluginInfoWithHost();

			m_UniqueID = m_PlugDesc->UniqueID;

			m_InputPortCount = m_PluginInfo.NumInputs;

			size_t lbl_length;

			const char *nm = m_PlugDesc->Name ? m_PlugDesc->Name : "LADSPA";
			lbl_length = strlen(nm);
			lbl_length = lbl_length > 255 ? 255 : lbl_length;
			memcpy(m_Name, nm, lbl_length);
			m_Name[lbl_length] = '\0';

			const char *mk = m_PlugDesc->Maker ? m_PlugDesc->Maker : "";
			lbl_length = strlen(mk);
			lbl_length = lbl_length > 255 ? 255 : lbl_length;
			memcpy(m_Maker, mk, lbl_length);
			m_Maker[lbl_length] = '\0';
		}

		return true;
	}
	return false;
}

void LADSPAPlugin::ClearPlugin(void)
{
	if (m_PlugDesc) {
		if (m_PlugDesc->deactivate) m_PlugDesc->deactivate(m_PlugInstHandle);
		if (m_PlugDesc->cleanup) m_PlugDesc->cleanup(m_PlugInstHandle);
		m_PlugDesc = NULL;
		m_PlugInstHandle = NULL;

		m_LADSPAInfo->DiscardDescriptorByID(m_UniqueID);
	}

	if (! m_SSMPluginReset)
	{
		m_Page = 2;
		m_UpdateInputs = true;
		m_UniqueID = 0;
		m_InputPortCount = 0;
		m_UnconnectedInputs = 0;
		strncpy(m_Name, "None", 5);
		strncpy(m_Maker, "None", 5);
		m_Name[4] = '\0';
		m_Maker[4] = '\0';
	}

	for(vector<LADSPA_Data*>::iterator i=m_LADSPABufVec.begin();
		i!=m_LADSPABufVec.end(); i++)
	{
		if (*i) delete[] (*i);
	}
	m_LADSPABufVec.clear();

	if (! m_SSMPluginReset)
	{
		RemoveAllInputs();
		RemoveAllOutputs();

		m_PluginInfo.NumInputs = 0;
		m_PluginInfo.NumOutputs = 0;
		m_PluginInfo.PortTips.clear();
		m_PluginInfo.PortTypes.clear();
	}

	m_PortID.clear();
	m_InputPortMin.clear();
	m_InputPortMax.clear();
	m_InputPortClamp.clear();

	if (! m_SSMPluginReset)
	{
		m_InputPortDefault.clear();
	}
}

void LADSPAPlugin::SyncPortTypes(void)
{
	m_PluginInfo.PortTypes.clear();
	m_PluginInfo.PortTypes.resize(m_PluginInfo.NumInputs + m_PluginInfo.NumOutputs, 0);
}

void LADSPAPlugin::ResetPortSettings(void)
{
	if (!m_PlugDesc || !m_HostInfo)
		return;

	for (int n=0; n<m_PluginInfo.NumInputs; n++)
	{
		if (n >= (int)m_PortID.size())
			break;

		float Max=1.0f, Min=-1.0f, Default=0.0f;
		int Port=m_PortID[n];

		if (!m_PlugDesc->PortRangeHints)
		{
			m_InputPortMin.push_back(Min);
			m_InputPortMax.push_back(Max);
			m_InputPortClamp.push_back(true);
			if (! m_SSMPluginReset)
				m_InputPortDefault.push_back(Default);
			continue;
		}

		LADSPA_PortRangeHintDescriptor HintDesc=m_PlugDesc->PortRangeHints[Port].HintDescriptor;
		if (LADSPA_IS_HINT_BOUNDED_BELOW(HintDesc))
		{
			Min=m_PlugDesc->PortRangeHints[Port].LowerBound;
			if (LADSPA_IS_HINT_SAMPLE_RATE(HintDesc))
			{
				Min*=m_HostInfo->SAMPLERATE;
			}
		}
		if (LADSPA_IS_HINT_BOUNDED_ABOVE(HintDesc))
		{
			Max=m_PlugDesc->PortRangeHints[Port].UpperBound;
			if (LADSPA_IS_HINT_SAMPLE_RATE(HintDesc))
			{
				Max*=m_HostInfo->SAMPLERATE;
			}
		}

#ifdef LADSPA_VERSION
		if (LADSPA_IS_HINT_HAS_DEFAULT(HintDesc)) {
			if (LADSPA_IS_HINT_DEFAULT_1(HintDesc)) {
				Default = 1.0f;
			} else if (LADSPA_IS_HINT_DEFAULT_100(HintDesc)) {
				Default = 100.0f;
			} else if (LADSPA_IS_HINT_DEFAULT_440(HintDesc)) {
				Default = 440.0f;
			} else {
				if (LADSPA_IS_HINT_DEFAULT_MINIMUM(HintDesc) &&
				    LADSPA_IS_HINT_BOUNDED_BELOW(HintDesc)) {
					Default=m_PlugDesc->PortRangeHints[Port].LowerBound;
				} else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(HintDesc) &&
				           LADSPA_IS_HINT_BOUNDED_ABOVE(HintDesc)) {
					Default=m_PlugDesc->PortRangeHints[Port].UpperBound;
				} else if (LADSPA_IS_HINT_BOUNDED_BELOW(HintDesc) &&
				           LADSPA_IS_HINT_BOUNDED_ABOVE(HintDesc)) {
					float lp = 0.0f, up = 0.0f;
					float min = m_PlugDesc->PortRangeHints[Port].LowerBound;
					float max = m_PlugDesc->PortRangeHints[Port].UpperBound;
					if (LADSPA_IS_HINT_DEFAULT_LOW(HintDesc)) {
						lp = 0.75f;
						up = 0.25f;
					} else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(HintDesc)) {
						lp = 0.5f;
						up = 0.5f;
					} else if (LADSPA_IS_HINT_DEFAULT_HIGH(HintDesc)) {
						lp = 0.25f;
						up = 0.75f;
					}

					if (LADSPA_IS_HINT_LOGARITHMIC(HintDesc)) {
						if (min==0.0f || max==0.0f) {
							Default = 0.0f;
						} else {
							bool neg_min = min < 0.0f ? true : false;
							bool neg_max = max < 0.0f ? true : false;

							if (!neg_min && !neg_max) {
								Default = exp(log(min) * lp + log(max) * up);
							} else if (neg_min && neg_max) {
								Default = -exp(log(-min) * lp + log(-max) * up);
							} else {
								Default = min * lp + max * up;
							}
						}
					} else {
						Default = min * lp + max * up;
					}
				}
				if (LADSPA_IS_HINT_SAMPLE_RATE(HintDesc)) {
					Default *= m_HostInfo->SAMPLERATE;
				}
				if (LADSPA_IS_HINT_INTEGER(HintDesc)) {
					Default = floorf(Default);
				}
			}
		}
#else
#warning ************************************
#warning Your LADSPA header is out of date!
#warning Please get the latest sdk from
#warning     www.ladspa.org
#warning Defaults will not be used.
#warning ************************************
		Default = 0.0f;
#endif

		m_InputPortMin.push_back(Min);
		m_InputPortMax.push_back(Max);
		m_InputPortClamp.push_back(true);

		if (! m_SSMPluginReset)
		{
			m_InputPortDefault.push_back(Default);
		}
	}
}

void LADSPAPlugin::SetGUIExports(void)
{
	if (!m_PlugDesc || !m_OutData.InputPortNames || !m_OutData.InputPortSettings)
		return;

	size_t lbl_length;
	char *lbl_start;

	lbl_start = m_OutData.InputPortNames;
	for (unsigned long p = 0; p < m_InputPortCount && p < m_MaxInputPortCount; p++) {
		if (p >= m_PortID.size() || p >= m_PluginInfo.PortTips.size() ||
		    p >= m_InputPortMin.size() || p >= m_InputPortMax.size() ||
		    p >= m_InputPortClamp.size() || p >= m_InputPortDefault.size())
			break;

		int Port = m_PortID[p];
		LADSPA_PortRangeHintDescriptor HintDesc = 0;
		if (m_PlugDesc->PortRangeHints)
			HintDesc = m_PlugDesc->PortRangeHints[Port].HintDescriptor;

		lbl_length = m_PluginInfo.PortTips[p].size();
		lbl_length = lbl_length > 255 ? 255 : lbl_length;
		strncpy(lbl_start, m_PluginInfo.PortTips[p].c_str(), lbl_length);
		lbl_start[lbl_length] = '\0';
		lbl_start += 256;

		m_OutData.InputPortSettings[p].Integer = LADSPA_IS_HINT_INTEGER(HintDesc);
		if (LADSPA_IS_HINT_LOGARITHMIC(HintDesc)) {
			if (LADSPA_IS_HINT_SAMPLE_RATE(HintDesc)) {
				m_OutData.InputPortSettings[p].LogBase = 2.0f;
			} else {
				m_OutData.InputPortSettings[p].LogBase = 10.0f;
			}
		} else {
			m_OutData.InputPortSettings[p].LogBase = 0.0f;
		}

		m_OutData.InputPortSettings[p].Min = m_InputPortMin[p];
		m_OutData.InputPortSettings[p].Max = m_InputPortMax[p];
		m_OutData.InputPortSettings[p].Clamp = m_InputPortClamp[p] ? true : false;
		m_OutData.InputPortDefaults[p] = m_InputPortDefault[p];
	}
}
