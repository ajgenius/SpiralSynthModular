/*  SpiralSound
 *  Copyleft (C) 2002 David Griffiths <dave@pawfal.org>
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

#include "SpiralPlugin.h"
#include "SampleFeed.h"

using namespace std;

SpiralPlugin::SpiralPlugin()
{
	m_Version=1;
	m_PluginInfo.Name="BasePlugin";
	m_PluginInfo.Width=100;
	m_PluginInfo.Height=100;
	m_PluginInfo.NumInputs=0;
	m_PluginInfo.NumOutputs=0;
	UpdateInfo=NULL;
	cb_Update=NULL;
	m_Parent=NULL;
	m_HostID=-1;
	m_IsTerminal=false;
	m_IsDead=false;
	m_AudioCH = new ChannelHandler;
	m_FeedCount=0;
	m_FeedUsers=0;
}

SpiralPlugin::~SpiralPlugin()
{
	RemoveAllOutputs();	
	RemoveAllInputs();	
	delete m_AudioCH;
	for (int n=0; n<m_FeedCount; n++)
		delete m_Feeds[n].Feed;
}

bool SpiralPlugin::Kill() 
{
	m_IsDead = true; 
	return true;
}

// Anchor the protocol's RTTI in libspiralcore so RTLD_LOCAL devices and
// the host share one identity, including with libc++ on macOS.
StablePortLayout::~StablePortLayout() {}

void SpiralPlugin::ResetPorts() 
{
	for (int n=0; n<m_PluginInfo.NumOutputs; n++)
	{
		m_Output[n]->Clear();
		m_Output[n]->Allocate(m_HostInfo->BUFSIZE);
	}
}

void SpiralPlugin::StreamOut(std::ostream &s)
{
	Spiral::Core::Description d;
	Describe(d);
	d.Write(s);
}

void SpiralPlugin::Reset() 
{
	ResetPorts();
}

PluginInfo &SpiralPlugin::Initialise(const HostInfo *Host)
{	
	m_HostInfo=Host;

        for (int n=0; n<m_PluginInfo.NumInputs; n++)
	{
		m_Input.push_back(NULL);
	}

	for (int n=0; n<m_PluginInfo.NumOutputs; n++)
	{
		m_Output.push_back(new Sample(Host->BUFSIZE));
	}

	for (int n=0; n<m_PluginInfo.NumInputs+m_PluginInfo.NumOutputs; n++)
 	{
 		m_PluginInfo.PortTypes.push_back(0);
 	}

	return m_PluginInfo;
}

bool SpiralPlugin::GetOutput(unsigned int n, Sample **s)
{
	if (n>=m_Output.size()) return false;
	*s = m_Output[n];

	return true;
}

bool SpiralPlugin::SetInput(unsigned int n, const Sample *s)
{
	if (n>=m_Input.size()) return false;
	m_Input[n]=s;
	return true;
}

void SpiralPlugin::UpdateChannelHandler()
{
    m_AudioCH->UpdateDataNow(m_HostInfo ? m_HostInfo->FRAME : 0);
}

void SpiralPlugin::StampOutputs(unsigned long frame)
{
	for (std::vector<Sample*>::iterator i=m_Output.begin(); i!=m_Output.end(); ++i)
		if (*i) (*i)->Stamp(frame);
}

void SpiralPlugin::AddOutput()
{
	Sample* NewSample = new Sample(m_HostInfo->BUFSIZE);
	m_Output.push_back(NewSample);
}

void SpiralPlugin::RemoveOutput()
{
	vector<Sample*>::iterator si=m_Output.end();
	si--;
	delete *si;
	m_Output.erase(si);
}

void SpiralPlugin::RemoveAllOutputs()
{
	for (vector<Sample*>::iterator i=m_Output.begin();
		 i!=m_Output.end(); i++)
	{
		delete *i;
	}

	m_Output.clear();
}

void SpiralPlugin::AddInput()
{
	m_Input.push_back(NULL);
}

void SpiralPlugin::RemoveInput()
{
	vector<const Sample*>::iterator si=m_Input.end();
	si--;
	m_Input.erase(si);
}

void SpiralPlugin::RemoveAllInputs()
{
	m_Input.clear();
}

void SpiralPlugin::UpdatePluginInfoWithHost()
{
	if (UpdateInfo!=NULL)
	{
		UpdateInfo(m_HostID,(void*)&m_PluginInfo);
	}
}
void SpiralPlugin::SetUpdateInfoCallback(int ID, void(*s)(int, void *))
{
	m_HostID=ID;
	UpdateInfo=s;
}

void SpiralPlugin::SetInPortType(PluginInfo &pinfo, int port, Sample::SampleType type)
{
    pinfo.PortTypes[port] = type;
}

void SpiralPlugin::SetOutPortType(PluginInfo &pinfo, int port, Sample::SampleType type)
{
    pinfo.PortTypes[port+m_PluginInfo.NumInputs] = type;
    GetOutputBuf(port)->setSampleType(type);
}

Spiral::Core::SampleFeed *SpiralPlugin::RequestFeed(bool output, int port)
{
	for (int n=0; n<m_FeedCount; n++)
	{
		if (m_Feeds[n].Output==output && m_Feeds[n].Port==port)
		{
			__sync_fetch_and_add(&m_Feeds[n].Users, 1);
			__sync_fetch_and_add(&m_FeedUsers, 1);
			return m_Feeds[n].Feed;
		}
	}
	if (m_FeedCount>=MaxFeeds || port<0) return NULL;

	FeedSlot &slot=m_Feeds[m_FeedCount];
	slot.Output=output;
	slot.Port=port;
	slot.Feed=new Spiral::Core::SampleFeed;
	slot.Users=1;
	__sync_synchronize();
	__sync_fetch_and_add(&m_FeedCount, 1);
	__sync_fetch_and_add(&m_FeedUsers, 1);
	return slot.Feed;
}

void SpiralPlugin::ReleaseFeed(Spiral::Core::SampleFeed *feed)
{
	for (int n=0; n<m_FeedCount; n++)
	{
		if (feed && m_Feeds[n].Feed==feed && m_Feeds[n].Users>0)
		{
			__sync_fetch_and_sub(&m_Feeds[n].Users, 1);
			__sync_fetch_and_sub(&m_FeedUsers, 1);
			return;
		}
	}
}

void SpiralPlugin::PublishHeldFeeds()
{
	const int count=m_FeedCount;
	__sync_synchronize();
	for (int n=0; n<count; n++)
	{
		const FeedSlot &slot=m_Feeds[n];
		if (!slot.Users) continue;
		const Sample *block=NULL;
		if (slot.Output)
		{
			if (slot.Port<(int)m_Output.size()) block=m_Output[slot.Port];
		}
		else if (slot.Port<(int)m_Input.size()) block=m_Input[slot.Port];
		if (block) slot.Feed->Publish(block->GetBuffer(), block->GetLength());
	}
}
