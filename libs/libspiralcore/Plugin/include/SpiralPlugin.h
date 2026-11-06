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

#ifndef SPIRALPLUGIN
#define SPIRALPLUGIN

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include "Sample.h"
using Spiral::Core::Sample;
using Spiral::Core::Linear;
using Spiral::Core::feq;
#include "ChannelHandler.h"
#include "Description.h"
using Spiral::Core::ChannelHandler;
#include "DeviceClassRegistry.h"

namespace Spiral
{
namespace Core
{
	class SampleFeed;
	class EventBuffer;
}
}
using Spiral::Core::SPIRAL_PLUGIN_TYPE_DEVICE;
using Spiral::Core::SPIRAL_PLUGIN_TYPE_EDITOR;
using Spiral::Core::SPIRAL_PLUGIN_TYPE_PAIRED;

static const float MAX_FREQ = 13000;

struct PluginInfo
{
	std::string Name;
	int Width;
	int Height;
	int NumInputs;
	int NumOutputs;
	std::vector<std::string> PortTips;
	std::vector<int> PortTypes;
	char BitMap[40][40][3];
};

struct HostInfo
{
	int    BUFSIZE;
	int    SAMPLERATE;
	
	/* obsolete - REMOVE SOON */
	int    FRAGSIZE;
	int    FRAGCOUNT;
	std::string OUTPUTFILE;
	std::string MIDIFILE;
	int    POLY;

        unsigned GUI_COLOUR;
        unsigned SCOPE_BG_COLOUR;
        unsigned SCOPE_FG_COLOUR;
        unsigned SCOPE_SEL_COLOUR;
        unsigned SCOPE_IND_COLOUR;
        unsigned SCOPE_MRK_COLOUR;
        unsigned GUICOL_Device;
        unsigned GUIDEVICE_Box;

	bool   PAUSED;
	std::string AUDIOCLIENT;

	unsigned long FRAME;
	bool   ROLLING;
};

/////////////////////////////////////////////////////////////////////

class StablePortLayout
{
public:
	virtual ~StablePortLayout();
	virtual int MovedInput(int port) const { return port; }
};

class SpiralPlugin
{
public:
	SpiralPlugin();
	virtual ~SpiralPlugin();

	virtual PluginInfo& Initialise(const HostInfo *Host);

	// execute the audio
	virtual void        Execute()=0;
	virtual bool         Kill();
	virtual void         Reset();

	// run the commands from the GUI
	virtual void        ExecuteCommands() {}

	virtual void        SetVoice(SpiralPlugin *first, unsigned voice, unsigned voices) {}

	virtual void        Describe(Spiral::Core::Description &d)=0;
	void                StreamOut(std::ostream &s);
	virtual void        Apply(Spiral::Core::Description::Reader &r)=0;
	virtual void        Migrate(Spiral::Core::Description &d) {}

	// stuff here gets saved in filename_files directory
	// you must return true if this feature is used.
	virtual bool	    SaveExternalFiles(const std::string &Dir) { return false; }
	virtual void	    LoadExternalFiles(const std::string &Dir, int withID=-1) {}
	virtual void        RestoreUnavailablePorts(int inputs, int outputs) {}

	virtual bool        GeneratePanel(const std::string &Directory, std::string &Document) { return false; }
	virtual unsigned    PanelGeneration() { return 0; }

	const PluginInfo& GetPluginInfo() const { return m_PluginInfo; }
	const HostInfo*     GetHostInfo() { return m_HostInfo; }
	bool                GetOutput(unsigned int n, Sample **s);
	bool                SetInput(unsigned int n, const Sample *s);
	const Sample*       GetInput(unsigned int n) { return m_Input[n]; }
	bool                SetEventInput(unsigned int n, const Spiral::Core::EventBuffer *e);
	const Spiral::Core::EventBuffer *GetEventInput(unsigned int n) const;
	Spiral::Core::EventBuffer *GetEventOutput(unsigned int n) const;
	bool                IsEventPort(bool input, unsigned int n) const;
	std::string 		GetName() { return m_PluginInfo.Name; }

	void UpdatePluginInfoWithHost();
	void SetInPortType(PluginInfo &pinfo, int port, Sample::SampleType type);
	void SetOutPortType(PluginInfo &pinfo, int port, Sample::SampleType type);

	// Callbacks to main engine. Should only called by plugin hosts.
	void SetUpdateInfoCallback(int ID, void(*s)(int, void *));
	void SetUpdateCallback(void (*s)(void*,bool m)) { cb_Update=s; }
	void SetParent(void *s) { m_Parent=s; }

	void UpdateChannelHandler();
	void StampOutputs(unsigned long frame);
	// is the plugin connected to an external device (oss/alsa/jack)
	bool IsTerminal() { return m_IsTerminal; }
	bool IsDead() { return m_IsDead; }
	 
	ChannelHandler *GetChannelHandler() { return m_AudioCH; }
	Spiral::Core::SampleFeed *RequestFeed(bool output, int port);
	void ReleaseFeed(Spiral::Core::SampleFeed *feed);
	void PublishFeeds() { if (m_FeedUsers) PublishHeldFeeds(); }

	virtual bool IsAudioDriver() { return false; }
protected:
	
    ChannelHandler *m_AudioCH;

	void  SetOutput(int n,int p, float s) 
		{ if (m_Output[n]) m_Output[n]->Set(p,s); }
		
	float GetInput(int n,int p)  
		{ if (m_Input[n]) return (*m_Input[n])[p]; else return 0.0; }
		
	void  SetOutputPitch(int n,int p, float s) 
		{ if (m_Output[n]) m_Output[n]->Set(p,(s/MAX_FREQ*2)-1.0f); }
		
	float GetInputPitch(int n,int p)
		{ if (m_Input[n]) return ((*m_Input[n])[p]+1.0f)*MAX_FREQ/2; else return 0.0; }

	void  MixOutput(int n,int p, float s)
		{ if (m_Output[n]) m_Output[n]->Set(p,s+(*m_Output[n])[p]); }
	
	bool InputExists(int n) { return m_Input[n]!=NULL; }
	bool OutputExists(int n) { return m_Output[n]!=NULL; }	

	void AddOutput();
	void RemoveOutput();
	void RemoveAllOutputs();

	void AddInput();
	void RemoveInput();
	void RemoveAllInputs();
		
	void ResetPorts();

	// the ID number assigned to us by ssm
	int GetID() { return m_HostID; }
	
	Sample* GetOutputBuf(int n) { return m_Output[n]; }	
	
	const HostInfo *m_HostInfo;
	PluginInfo      m_PluginInfo;
	int             m_Version;

	// needed for jack
	void (*cb_Update)(void*o ,bool m);	
	void  *m_Parent;

	bool   m_IsTerminal;
	bool   m_IsDead;
	
private:

	std::vector<const Sample*> m_Input;
	std::vector<Sample*> m_Output;
	std::vector<const Spiral::Core::EventBuffer*> m_EventInput;
	std::vector<Spiral::Core::EventBuffer*> m_EventOutput;

	void (*UpdateInfo)(int n,void *);	
	int    m_HostID;	

	struct FeedSlot
	{
		bool Output;
		int Port;
		Spiral::Core::SampleFeed *Feed;
		volatile int Users;
	};
	static const int MaxFeeds = 4;
	FeedSlot m_Feeds[MaxFeeds];
	volatile int m_FeedCount;
	volatile int m_FeedUsers;
	void PublishHeldFeeds();
};

#endif
