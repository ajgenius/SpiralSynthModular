/*  SpiralSound
 *  Copyleft (C) 2001 David Griffiths <dave@pawfal.org>
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
#include "SpiralLoopPlugin.h"
#include "SpiralIcon.xpm"
#include "NoteTable.h"

#include "RiffWav.h"

using namespace std;
using namespace Spiral::Core;

static const float TRIG_THRESH = 0.1;
static const int   RECBUFFERSIZE = 16384;
static const float RECORD_GAIN = 1.0f;

#include <config.h>

extern "C" {
const char *SpiralPlugin_GetHostVersion()
{
	return PACKAGE_VERSION;
}

const char *SpiralPlugin_GetHostABI()
{
	return SSM_HOST_ABI;
}

SpiralPlugin* SpiralPlugin_CreateInstance()
{
	return new SpiralLoopPlugin;
}

int SpiralPlugin_GetType()
{
	return SPIRAL_PLUGIN_TYPE_DEVICE;
}


const char** SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}

int SpiralPlugin_GetID()
{
	return 0x001a;
}

string SpiralPlugin_GetName()
{
	return "SpiralLoop";
}

string SpiralPlugin_GetGroupName()
{
	return "Delay/Sampling";
}
}

///////////////////////////////////////////////////////

SpiralLoopPlugin::SpiralLoopPlugin() : 
m_Id(0),
m_Pos(0),
m_IntPos(0),
m_OldPos(0),
m_PlayBufPos(0),
m_Playing(true),
m_Recording(false),
m_DelMe(false),
m_LoopPoint(0),
m_RecordingSource(NULL),
m_FirstRecord(true),
m_FixedRecord(false),
m_RecLength(0),
m_TickTime(0),
m_TickCurrent(0),
m_TickOutput(1.0f),
m_Triggered(false)
{
	m_PluginInfo.Name="SpiralLoop";
	m_PluginInfo.Width=300;
	m_PluginInfo.Height=320;
	m_PluginInfo.NumInputs=2;
	m_PluginInfo.NumOutputs=10;		
	m_PluginInfo.PortTips.push_back("Input");
	m_PluginInfo.PortTips.push_back("Play Trigger");	
	m_PluginInfo.PortTips.push_back("Output");
	m_PluginInfo.PortTips.push_back("Clock");
	m_PluginInfo.PortTips.push_back("LoopTrigger 0");
	m_PluginInfo.PortTips.push_back("LoopTrigger 1");
	m_PluginInfo.PortTips.push_back("LoopTrigger 2");
	m_PluginInfo.PortTips.push_back("LoopTrigger 3");
	m_PluginInfo.PortTips.push_back("LoopTrigger 4");
	m_PluginInfo.PortTips.push_back("LoopTrigger 5");
	m_PluginInfo.PortTips.push_back("LoopTrigger 6");
	m_PluginInfo.PortTips.push_back("LoopTrigger 7");			
	
	SetString(DeviceName, SpiralPlugin_GetName());
	SetUnsigned(DeviceVersion, 2);

	m_TicksPerLoop = SignedProperty(DefaultLinearFlags, 64, 1, 1024, 1, 1);
	m_Volume = FloatProperty(DefaultLinearFlags, 1.0, 0, 2, 0.001, 0.01);
	m_Speed = FloatProperty(DefaultLinearFlags, 1.0, 0, 4, 0.001, 0.01);

	RegisterSharedProperty(m_TicksPerLoop, "TicksPerLoop", "Ticks Per Loop");
	RegisterSharedProperty(m_Volume, "Volume", "Volume");
	RegisterSharedProperty(m_Speed, "Speed", "Speed");

	m_Triggers = VectorProperty(Property::WriteOnly);
	RegisterSharedPropertyWithKey(m_Triggers, "TRIGGERS", "Triggers", "Trigger points");
	m_AudioCH->Register("Length",&m_GUIArgs.Length);
	m_AudioCH->Register("Start",&m_GUIArgs.Start);
	m_AudioCH->Register("End",&m_GUIArgs.End);
	m_AudioCH->Register("Pos",&m_Pos,ChannelHandler::OUTPUT);
	m_AudioCH->RegisterData("Name",ChannelHandler::INPUT,&m_GUIArgs.Name,sizeof(m_GUIArgs.Name));
	m_AudioCH->RegisterData("SampleBuffer",ChannelHandler::OUTPUT_REQUEST,&m_SampleBuffer,TRANSBUF_SIZE);
	m_AudioCH->Register("SampleSize",&m_SampleSize,ChannelHandler::OUTPUT_REQUEST);

	m_Version=2;
}

SpiralLoopPlugin::~SpiralLoopPlugin()
{
}

PluginInfo &SpiralLoopPlugin::Initialise(const HostInfo *Host)
{	
	return SpiralPlugin::Initialise(Host);
}



void SpiralLoopPlugin::Reset()
{
	ResetPorts();
	m_TickOutput=-m_TickOutput;
	m_TickTime=m_StoreBuffer.GetLength()/AsSigned(m_TicksPerLoop);
	m_TickCurrent=0;
}

void SpiralLoopPlugin::Execute()
{
	if (InputExists(0)) SetRecordingSource(GetInput(0)->GetBuffer());
	else SetRecordingSource(NULL);

	for (int n=0; n<8; n++) GetOutputBuf(n+1)->Zero();

	// get the triggers active this frame
	for (unsigned int i=0; i<m_Triggers.size(); i++)
	{
		if (m_Pos > AsFloat(TriggerTime(i))*m_StoreBuffer.GetLength() &&
			!m_Fired[i])
		{
			GetOutputBuf(AsSigned(TriggerChannel(i))+2)->Set(1);
			m_Fired[i]=true;
		}
	}

	if (GetOutput(*GetOutputBuf(0)))
	{
		// if it's looped - reset the triggers
		for (unsigned int i=0; i<m_Fired.size(); i++) m_Fired[i]=false;
		
		m_TickCurrent=m_TickTime;
	}	

	if (GetInput(1,0)>TRIG_THRESH) 
	{
		if (!m_Triggered)
		{
			Trigger();		
			m_Triggered=true;
		}
	}
	else m_Triggered=false;
	
	m_TickCurrent+=m_HostInfo->BUFSIZE;
	if (m_TickCurrent>=m_TickTime)
	{
		m_TickOutput=-m_TickOutput;
		m_TickTime=m_StoreBuffer.GetLength()/AsSigned(m_TicksPerLoop);
		m_TickCurrent=0;
	}
	
	GetOutputBuf(1)->Set(m_TickOutput);
}

void SpiralLoopPlugin::ExecuteCommands()
{
	if (m_AudioCH->IsCommandWaiting())
	{
		switch(m_AudioCH->GetCommand())
		{
		case START			: SetPlaying(true); break;
		case STOP			: SetPlaying(false); break;
		case RECORD			: Clear(); Record(true); break;
		case OVERDUB		: Record(true); break;
		case ENDRECORD		: Record(false); break;		
		case LOAD			: LoadWav(m_GUIArgs.Name); break;
		case SAVE			: SaveWav(m_GUIArgs.Name); break;
		case CUT			: Cut(m_GUIArgs.Start, m_GUIArgs.End); break;
		case COPY			: Copy(m_GUIArgs.Start, m_GUIArgs.End); break;
		case PASTE			: Paste(m_GUIArgs.Start); break;
		case PASTEMIX		: PasteMix(m_GUIArgs.Start); break;
		case ZERO_RANGE		: ZeroRange(m_GUIArgs.Start, m_GUIArgs.End); break;
		case REVERSE_RANGE	: ReverseRange(m_GUIArgs.Start, m_GUIArgs.End); break;
		case SELECT_ALL		: SelectAll(); break;
		case DOUBLE			: Double(); break;
		case HALF			: Halve(); break;
		case MOVE			: Move(m_GUIArgs.Start); break;
		case CROP			: Crop(); break;
		case KEEPDUB		: MixDub(); break;
		case UNDODUB		: ClearDub(); break;
		case CHANGE_LENGTH	: m_LoopPoint=(int)(m_StoreBuffer.GetLength()*m_GUIArgs.Length); break;
		case NEW_TRIGGER	: 
			{
				if ((int)m_Triggers.size()!=m_GUIArgs.Start) cerr<<"no of triggers error!"<<endl;
				AddTrigger(m_GUIArgs.End, m_GUIArgs.Length); break;
			}
		case UPDATE_TRIGGER	: 
			{
				AsSigned(TriggerChannel(m_GUIArgs.Start)) = m_GUIArgs.End;
				AsFloat(TriggerTime(m_GUIArgs.Start))    = m_GUIArgs.Length;			
			}
		case GETSAMPLE    	: 
			{
				m_AudioCH->SetupBulkTransfer((void*)m_StoreBuffer.GetBuffer());
				m_SampleSize=m_StoreBuffer.GetLength();
			} break;
		}
	}
}

void SpiralLoopPlugin::Describe(Spiral::Core::Description &d)
{
	d.Value(DeviceVersion).Separator(" "); 
	d.Value(m_LoopPoint).Key("LOOPPOINT", "").Separator(" ").Value(m_Speed).Separator(" ").Value(m_Volume).Separator(" ").Value(m_TicksPerLoop).Separator(" ");
	d.Value(m_Triggers.size()).Separator(" ");
	for (unsigned int i=0; i<m_Triggers.size(); i++)
	{
		d.Value(TriggerChannel(i)).Separator(" ").Value(TriggerTime(i)).Separator(" ");
	}
}

void SpiralLoopPlugin::Apply(Spiral::Core::Description::Reader &r)
{
	int version;
	r.Value(version);
	if (r.Named())
		r.At(r.Find("LOOPPOINT"), m_LoopPoint);
	else
		r.Value(m_LoopPoint);

	r.Value(m_Speed).Value(m_Volume).Value(m_TicksPerLoop);
	int size;
	r.Count(m_Triggers, size);
	
	for (int n=0; n<size; n++)
	{
		AddTrigger(0, 0);
		r.Value(TriggerChannel(n)).Value(TriggerTime(n));
	}
}
	
bool SpiralLoopPlugin::SaveExternalFiles(const string &Dir)
{
	char temp[256];
	sprintf(temp,"%sSpiralLoopSample%d.wav",Dir.c_str(),SpiralPlugin_GetID());	
	SaveWav(temp);
	return true;
}

void SpiralLoopPlugin::LoadExternalFiles(const string &Dir, int withID)
{
	char temp[256];
	sprintf(temp,"%sSpiralLoopSample%d.wav",Dir.c_str(),withID == -1 ? SpiralPlugin_GetID() : withID);
	LoadWav(temp);
}


void SpiralLoopPlugin::LoadWav(const char *Filename)
{
	WavFile wav;
	if (wav.Open(Filename, WavFile::READ))
	{
		//Clear();
		AllocateMem(wav.GetSize());		
		wav.Load(m_StoreBuffer);	
	}
}

void SpiralLoopPlugin::SaveWav(const char *Filename)
{
	WavFile wav;
	if (wav.Open(Filename, WavFile::WRITE, WavFile::MONO))
	{
		wav.Save(m_StoreBuffer);
	}
	
	m_Sample=Filename;
}

bool SpiralLoopPlugin::GetOutput(Sample &data)
{
	if (!m_Recording && !m_Playing)
	{
		return false;
	}
	
	if (!m_Recording && m_StoreBuffer.GetLength()==0)
	{
		return false;
	}
	
	if (m_FixedRecord && m_DubBuffer.GetLength()!=m_StoreBuffer.GetLength())
	{
		cerr<<"eek! dub and store buffers don't match!"<<endl;
	}
	
	if (m_Recording)
	{	
		RecordBuf(m_Pos, data.GetLength());
			
		if (!m_StoreBuffer.GetLength())
		{
			return false;
		}	
	}

	int Pos;
	bool ret=false;
	
	for (int n=0; n<data.GetLength(); n++)
	{		
		Pos=static_cast<int>(m_Pos);
				
		// brute force fix
		if (Pos>0 && Pos<m_LoopPoint)
		{
			data.Set(n,(m_StoreBuffer[m_Pos]+m_DubBuffer[m_Pos])*AsFloat(m_Volume));
		}
		else data.Set(n,0);
		
		m_Pos+=AsFloat(m_Speed);
		
		if (static_cast<int>(m_Pos)>=m_LoopPoint)
		{
			ret=true;
			m_Pos=0;
		}
	}
	
	m_IntPos=static_cast<int>(m_Pos);
	
	return ret;
}

void SpiralLoopPlugin::AllocateMem(int Length)
{		
	// We might need to keep these values (if loading workspace)
	if (m_LoopPoint>Length) m_LoopPoint=Length;
	if (m_Pos>Length) m_Pos=0;
	
	if (m_LoopPoint==0) m_LoopPoint=Length;
	
	if (!m_StoreBuffer.Allocate(Length) || 
	    !m_DubBuffer.Allocate(Length))
	{
		cerr<<"AllocateMem can't allocate any more memory!"<<endl;
		Clear();
	}
}

void SpiralLoopPlugin::Clear()
{
	m_StoreBuffer.Clear();
	m_DubBuffer.Clear();
	m_FixedRecord=false;
	m_FirstRecord=true;
	m_LoopPoint=0;
}

void SpiralLoopPlugin::RecordBuf(float Pos, int Length)
{
	if (!m_RecordingSource) return;

	if (m_FirstRecord)
	{
		// Find out if we want a fixed length record
		// based on the last sample length, or not
		if (m_StoreBuffer.GetLength())
		{
			m_FixedRecord=true;
		}
		else
		{
			m_FixedRecord=false;
			m_RecBuffer.Allocate(RECBUFFERSIZE);
			m_StoreBuffer.Clear();
			m_RecPos=0;
		}

		m_FirstRecord=false;
		m_RecLength=0;
		m_OldPos=Pos;
	}
	
	if (m_FixedRecord)
	{
		m_RecLength=m_LoopPoint;
				
		if (Pos>=m_StoreBuffer.GetLength())
		{
			Pos=0;
		}
			
		for (int n=0; n<Length; n++)
		{			
			// just add directly to the old buffer
			float temp=m_DubBuffer[static_cast<int>(Pos)]+m_RecordingSource[n]*RECORD_GAIN;
			
			// fill in all the samples between the speed jump with the same value
			m_DubBuffer.Set((int)Pos,temp);
			
			for (int i=static_cast<int>(m_OldPos); i<=static_cast<int>(Pos); i++)
			{
				m_DubBuffer.Set(i,temp);
			}
						
			m_OldPos=Pos;
			Pos+=AsFloat(m_Speed);
			
			if (Pos>=m_StoreBuffer.GetLength())
			{
				Pos-=m_StoreBuffer.GetLength();				
				
				// remember to fill up to the end of the last buffer
				for (int i=static_cast<int>(m_OldPos); i<m_StoreBuffer.GetLength(); i++)
				{
					m_DubBuffer.Set(i,temp);
				}
				
				// and the beggining of this one
				for (int i=0; i<Pos; i++)
				{
					m_DubBuffer.Set(i,temp);
				}
				
				m_OldPos=0;	
			}
			
		}
	}
	else
	{
		for (int n=0; n<Length; n++)
		{							
			// see if we need a new buffer
			if (m_RecPos>=RECBUFFERSIZE)
			{
				// put the two buffers together
				m_StoreBuffer.Add(m_RecBuffer);				
				m_RecPos=0;
			}
				
			m_RecBuffer.Set(m_RecPos,m_RecordingSource[n]*RECORD_GAIN);
			
			m_RecLength++;
			m_RecPos++;
		}
	}
}

void SpiralLoopPlugin::EndRecordBuf()
{
	m_FirstRecord=true;	
	m_LoopPoint=m_StoreBuffer.GetLength();
	
	if (!m_FixedRecord)
	{
		// reallocate the hold buffer for the new size
		// (if the size has changed)
		m_DubBuffer.Allocate(m_LoopPoint);
	}
}

void SpiralLoopPlugin::Crop()
{
	if (m_LoopPoint<m_StoreBuffer.GetLength())
	{
		m_StoreBuffer.CropTo(m_LoopPoint);
		m_DubBuffer.CropTo(m_LoopPoint);
	}
}

void SpiralLoopPlugin::Double()
{
	Crop();
	
	m_StoreBuffer.Add(m_StoreBuffer);
	m_DubBuffer.Add(m_DubBuffer);
	m_LoopPoint=m_StoreBuffer.GetLength();
}

void SpiralLoopPlugin::MatchLength(int Len)
{
	// set the length, and make sure enough data is allocated
	if (m_StoreBuffer.GetLength()>Len)
	{
		SetLength(Len);
		return;
	}
	else
	{
		// if it's empty
		if (!m_StoreBuffer.GetLength())
		{			
			AllocateMem(Len);
			m_StoreBuffer.Zero();
		}
		else 
		// there is something in the buffer already, but we need to
		// add on some extra data to make the length the same
		{
			int ExtraLen=Len-m_StoreBuffer.GetLength();
			m_StoreBuffer.Expand(ExtraLen);
			m_DubBuffer.Expand(ExtraLen);		
		}
	}
}

void SpiralLoopPlugin::Cut(int Start, int End)
{ 
	m_StoreBuffer.GetRegion(m_CopyBuffer,Start,End);
	m_StoreBuffer.Remove(Start,End);
	
	if (m_StoreBuffer.GetLength()<m_LoopPoint)
	{
		m_LoopPoint=m_StoreBuffer.GetLength();
	}
	if (m_Pos>m_LoopPoint)
	{
		m_Pos=0;
	}
	
	m_DubBuffer.Allocate(m_StoreBuffer.GetLength());
}

void SpiralLoopPlugin::Copy(int Start, int End)
{ 
	m_StoreBuffer.GetRegion(m_CopyBuffer,Start,End); 
}

void SpiralLoopPlugin::Paste(int Start)
{ 
	m_StoreBuffer.Insert(m_CopyBuffer,Start); 
	
	if (m_StoreBuffer.GetLength()<m_LoopPoint)
	{
		m_LoopPoint=m_StoreBuffer.GetLength();
	}
	if (m_Pos>m_LoopPoint)
	{
		m_Pos=0;
	}
	
	m_DubBuffer.Allocate(m_StoreBuffer.GetLength());
}

void SpiralLoopPlugin::PasteMix(int Start)
{ 
	m_StoreBuffer.Mix(m_CopyBuffer,Start); 
}

void SpiralLoopPlugin::ZeroRange(int Start, int End)
{
	for (int n=Start; n<End; n++)
	{
		m_StoreBuffer.Set(n,0);
	}
}

void SpiralLoopPlugin::ReverseRange(int Start, int End)
{
	m_StoreBuffer.Reverse(Start,End);
}

void SpiralLoopPlugin::Halve()
{
	m_StoreBuffer.Shrink(m_StoreBuffer.GetLength()/2);
	m_DubBuffer.Shrink(m_DubBuffer.GetLength()/2);

	if (m_StoreBuffer.GetLength()<m_LoopPoint)
	{
		m_LoopPoint=m_StoreBuffer.GetLength();
	}
	if (m_Pos>m_LoopPoint)
	{
		m_Pos=0;
	}
}

void SpiralLoopPlugin::SelectAll()
{
}

void SpiralLoopPlugin::Move(int Start)
{
	m_StoreBuffer.Move(Start);
}
