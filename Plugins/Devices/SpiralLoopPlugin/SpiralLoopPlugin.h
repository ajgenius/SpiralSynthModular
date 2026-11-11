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

#include "Device.h"

#ifndef SpiralLoopPLUGIN
#define SpiralLoopPLUGIN

static const int TRANSBUF_SIZE = 0x10000;
static const int MAX_TRIGGERS = 32;

struct SampleDesc
{
	std::string Pathname;
	float  Volume;
	float  Pitch;
	float  PitchMod;
	bool   Loop;
	int    Note;
	bool   TriggerUp;	
	float  SamplePos;
	int    SampleRate;
	bool   Stereo;
};

class SpiralLoopPlugin : public Spiral::Core::Device
{
public:
 	SpiralLoopPlugin();
	virtual ~SpiralLoopPlugin();
	
	void AddTrigger(int Channel, float Time)
	{
		const int n = AsSigned(m_TriggerCount);
		if (n >= MAX_TRIGGERS) return;
		AsSigned(TriggerChannel(n)) = Channel;
		AsFloat(TriggerTime(n)) = Time;
		m_Fired[n] = false;
		AsSigned(m_TriggerCount) = n + 1;
	}
	unsigned int TriggerCount() { return AsSigned(m_TriggerCount); }
	Spiral::Core::SignedProperty &TriggerChannel(int n)
	{ return *(Spiral::Core::SignedProperty *)(*(Spiral::Core::VectorProperty *)m_Triggers[n])[0]; }
	Spiral::Core::FloatProperty &TriggerTime(int n)
	{ return *(Spiral::Core::FloatProperty *)(*(Spiral::Core::VectorProperty *)m_Triggers[n])[1]; }
	
	virtual PluginInfo &Initialise(const HostInfo *Host);
	virtual void Execute();
	virtual void Reset();

	virtual void ExecuteCommands();
	virtual void Describe(Spiral::Core::Description &d);
	virtual void Apply(Spiral::Core::Description::Reader &r);
	virtual bool SaveExternalFiles(const std::string &Dir);
	virtual void LoadExternalFiles(const std::string &Dir, int withID=-1);
	
	enum GUICommands{NONE,START,STOP,RECORD,OVERDUB,ENDRECORD,LOAD,SAVE,CUT,
					 COPY,PASTE,PASTEMIX,ZERO_RANGE,REVERSE_RANGE,SELECT_ALL,DOUBLE,HALF,
					 MOVE,CROP,KEEPDUB,UNDODUB,CHANGE_LENGTH,NEW_TRIGGER,
					 GETSAMPLE,SEEK};	
	
	void LoadWav(const char *Filename);
	void SaveWav(const char *Filename);
	bool GetOutput(Sample &data);
	void AllocateMem(int Length);
	void Clear();
	void MixDub()   { m_StoreBuffer.Mix(m_DubBuffer,0); ClearDub(); }
	void ClearDub() { m_DubBuffer.Zero(); }
	void Double();
	void MatchLength(int Len);
	void Crop();
	
	void SetId(int Id)       				{ m_Id=Id; }
	void SetPos(int Pos)     				{ AsFloat(m_Pos)=Pos; }
	void SetLength(int Len)  				{ AsSigned(m_LoopPoint)=Len; }
	void SetSpeed(float Speed)				{ AsFloat(m_Speed)=Speed; }
	void SetVolume(float Vol)  				{ AsFloat(m_Volume)=Vol; }
	void SetPlaying(bool Playing) 			{ m_Playing=Playing; }
	void SetRecordingSource(const float *s) { m_RecordingSource=s; }
	void Record(bool r) 					{ m_Recording=r; if (!r) EndRecordBuf(); }
	void DelMe()              				{ m_DelMe=true; }
	void Trigger()							{ AsFloat(m_Pos)=0; SetPlaying(true); }
	
	const bool   IsPlaying() 				{return m_Playing;}
	const float *GetLoopPtr() 				{return m_StoreBuffer.GetBuffer();}
	const int    GetId() 					{return m_Id;}
	const int    GetLoopLength() 			{return AsSigned(m_LoopPoint);}
	float 		*GetPosPtr() 				{return &AsFloat(m_Pos);}
	const int    GetTotalLength() 			{assert(m_StoreBuffer.GetLength()==m_DubBuffer.GetLength()); return m_StoreBuffer.GetLength();}
	const bool   Delete()   				{return m_DelMe; }
	const float  GetSpeed() 				{return AsFloat(m_Speed);}
	const float  GetVolume()     			{ return AsFloat(m_Volume); }
	const float  GetCurrentAngle() 			{ return AsSigned(m_LoopPoint)?(AsFloat(m_Pos)/AsSigned(m_LoopPoint))*360.0f:0; }
	const std::string& GetSampleName()  			{ return m_Sample; }
	
	void Cut(int Start, int End);
	void Copy(int Start, int End);
	void Paste(int Start);
	void PasteMix(int Start);
	
	void ZeroRange(int Start, int End);
	void ReverseRange(int Start, int End);
	void Halve();
	void SelectAll();
	void Move(int Start);

	Sample *GetStoreBuffer() { return &m_StoreBuffer; }
		
private:

	struct GUIArgs
	{
		long  Start;
		long  End;
		float Length;
		char  Name[256];
	};
	
	GUIArgs m_GUIArgs;
	
	void RecordBuf(float Pos, int Length);
	void EndRecordBuf();

	int    m_Id;
	Spiral::Core::FloatProperty m_Pos;
	int    m_IntPos;
	float  m_OldPos;
	int    m_PlayBufPos;
	bool   m_Playing;
	bool   m_Recording;
	bool   m_DelMe;
	Spiral::Core::SignedProperty m_LoopPoint;
	Spiral::Core::FloatProperty  m_Speed;
	Spiral::Core::FloatProperty  m_Volume;
	const float *m_RecordingSource;
	Sample m_StoreBuffer;
	Sample m_DubBuffer;
	Sample m_RecBuffer;
	Sample m_CopyBuffer;
	int    m_RecPos;
	
	bool   m_FirstRecord;
	bool   m_FixedRecord;
	int    m_RecLength;
		
	int    m_TickTime;
	int    m_TickCurrent;
	Spiral::Core::SignedProperty    m_TicksPerLoop;
	float  m_TickOutput;
	
	bool m_Triggered;
	
	char  m_SampleBuffer[TRANSBUF_SIZE];
	long  m_SampleSize;
	
	Spiral::Core::VectorProperty m_Triggers;
	bool m_Fired[MAX_TRIGGERS];
	Spiral::Core::SignedProperty m_TriggerCount;

	std::string m_Sample;
};

#endif
