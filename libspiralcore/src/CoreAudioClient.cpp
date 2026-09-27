#include "CoreAudioClient.h"
#include <algorithm>
#include <cstring>
#include <iostream>

using namespace spiralcore;

namespace
{
	const AudioObjectPropertySelector Properties[] = {
		kAudioDevicePropertyNominalSampleRate,
		kAudioDevicePropertyBufferFrameSize,
		kAudioDevicePropertyDeviceIsAlive
	};

	AudioObjectPropertyAddress Address(AudioObjectPropertySelector selector)
	{
		AudioObjectPropertyAddress address = {selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster};

		return address;
	}

	bool GetDevice(const std::string &name, bool input, AudioDeviceID &device)
	{
		if (name.empty() || name=="default")
		{
			AudioObjectPropertyAddress address=Address(input ? kAudioHardwarePropertyDefaultInputDevice : kAudioHardwarePropertyDefaultOutputDevice);
			UInt32 size=sizeof(device);
			return AudioObjectGetPropertyData(kAudioObjectSystemObject,&address,0,NULL,&size,&device)==noErr && device!=kAudioObjectUnknown;
		}

		CFStringRef uid=CFStringCreateWithCString(NULL,name.c_str(),kCFStringEncodingUTF8);
		if (!uid) return false;

		AudioValueTranslation translation={&uid,sizeof(uid),&device,sizeof(device)};

		AudioObjectPropertyAddress address=Address(kAudioHardwarePropertyTranslateUIDToDevice);
		UInt32 size=sizeof(translation);

		OSStatus result=AudioObjectGetPropertyData(kAudioObjectSystemObject,&address,0,NULL,&size,&translation);
		CFRelease(uid);
		return result==noErr && device!=kAudioObjectUnknown;
	}

}

CoreAudioClient::CoreAudioClient() :
	m_Unit(NULL), m_Device(kAudioObjectUnknown), m_Inputs(0), m_Outputs(0),
	m_Capacity(0), m_ProcessFrames(0), m_Frames(0), m_Rate(0), m_Attached(0),
	m_Started(false), m_Listeners(0), m_Playback(NULL), m_Run(NULL), m_Context(NULL)
{
}

CoreAudioClient::~CoreAudioClient()
{
	Detach();
}

void CoreAudioClient::SetCallback(void (*run)(void *, unsigned int), void *context)
{
	m_Run=run;
	m_Context=context;
}

void CoreAudioClient::RefreshFormat()
{
	UInt32 frames=0;
	Float64 rate=0;
	UInt32 size=sizeof(frames);
	AudioObjectPropertyAddress address=Address(kAudioDevicePropertyBufferFrameSize);
	if (AudioObjectGetPropertyData(m_Device,&address,0,NULL,&size,&frames)==noErr)
		__sync_lock_test_and_set(&m_Frames,frames);

	address=Address(kAudioDevicePropertyNominalSampleRate);
	size=sizeof(rate);
	if (AudioObjectGetPropertyData(m_Device,&address,0,NULL,&size,&rate)==noErr)
		__sync_lock_test_and_set(&m_Rate,unsigned(rate));

}

bool CoreAudioClient::SetFormat(AudioUnitScope scope, AudioUnitElement element, unsigned channels)
{
	AudioStreamBasicDescription format;
	memset(&format,0,sizeof(format));
	format.mSampleRate=GetSampleRate();
	format.mFormatID=kAudioFormatLinearPCM;
	format.mFormatFlags=kAudioFormatFlagsNativeFloatPacked;
	format.mBytesPerPacket=format.mBytesPerFrame=sizeof(float)*channels;
	format.mFramesPerPacket=1;
	format.mChannelsPerFrame=channels;
	format.mBitsPerChannel=32;
	return AudioUnitSetProperty(m_Unit,kAudioUnitProperty_StreamFormat,scope,element,&format,sizeof(format))==noErr;
}

bool CoreAudioClient::Attach(const std::string &device, const AudioClientOptions &options)
{
	Detach();
	if ((!options.InChannels && !options.OutChannels) || options.InChannels>2 || options.OutChannels>2)
		return false;

	if (!GetDevice(device,!options.OutChannels,m_Device)) return false;

	// One HAL unit uses one hardware clock. Separate default input/output
	// devices require a user-selected aggregate device for duplex operation.
	if (options.InChannels && options.OutChannels && (device.empty() || device=="default"))
	{
		AudioDeviceID input=kAudioObjectUnknown;
		if (!GetDevice(device,true,input) || input!=m_Device)
		{
			std::cerr << "CoreAudio: duplex requires one device; select an aggregate device UID" << std::endl;
			return false;
		}

	}

	m_Inputs=options.InChannels;
	m_Outputs=options.OutChannels;
	RefreshFormat();
	if (!GetBufferSize() || !GetSampleRate()) return false;

	AudioComponentDescription description;
	memset(&description,0,sizeof(description));
	description.componentType=kAudioUnitType_Output;
	description.componentSubType=kAudioUnitSubType_HALOutput;
	description.componentManufacturer=kAudioUnitManufacturer_Apple;
	AudioComponent component=AudioComponentFindNext(NULL,&description);
	if (!component || AudioComponentInstanceNew(component,&m_Unit)!=noErr) return false;

	UInt32 input=m_Inputs ? 1 : 0, output=m_Outputs ? 1 : 0;

	OSStatus result=AudioUnitSetProperty(m_Unit,kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Input,1,&input,sizeof(input));
	if (result==noErr) result=AudioUnitSetProperty(m_Unit,kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Output,0,&output,sizeof(output));

	if (result==noErr) result=AudioUnitSetProperty(m_Unit,kAudioOutputUnitProperty_CurrentDevice,kAudioUnitScope_Global,0,&m_Device,sizeof(m_Device));

	if (result!=noErr || (m_Inputs && !SetFormat(kAudioUnitScope_Output,1,m_Inputs)) ||
		(m_Outputs && !SetFormat(kAudioUnitScope_Input,0,m_Outputs)))
	{
		Detach();
		return false;
	}

	UInt32 maximum=0, size=sizeof(maximum);
	AudioUnitGetProperty(m_Unit,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,&maximum,&size);
	m_Capacity=std::max(unsigned(GetBufferSize()),unsigned(maximum));
	m_Capture.assign(m_Capacity*m_Inputs,0);
	AURenderCallbackStruct callback={Process,this};

	result=AudioUnitSetProperty(m_Unit,m_Outputs ? kAudioUnitProperty_SetRenderCallback : kAudioOutputUnitProperty_SetInputCallback,
		m_Outputs ? kAudioUnitScope_Input : kAudioUnitScope_Global,0,&callback,sizeof(callback));
	if (result!=noErr || AudioUnitInitialize(m_Unit)!=noErr)
	{
		Detach();
		return false;
	}

	for (unsigned n=0; n<sizeof(Properties)/sizeof(Properties[0]); ++n)
	{
		AudioObjectPropertyAddress address=Address(Properties[n]);
		if (AudioObjectAddPropertyListener(m_Device,&address,DeviceChanged,this)!=noErr)
		{
			Detach();
			return false;
		}

		++m_Listeners;
	}

	__sync_lock_test_and_set(&m_Attached,1);
	return true;
}

bool CoreAudioClient::Start()
{
	if (!IsAttached()) return false;

	if (m_Started) return true;

	if (AudioOutputUnitStart(m_Unit)!=noErr)
	{
		Detach();
		return false;
	}

	m_Started=true;
	return true;
}

void CoreAudioClient::Detach()
{
	bool notify=IsAttached();
	if (m_Unit)
	{
		// Only the control thread stops/disposes the unit. Callback buffers and
		// context remain valid until CoreAudio has stopped invoking it.
		if (m_Started) AudioOutputUnitStop(m_Unit);

		for (unsigned n=0; n<m_Listeners; ++n)
		{
			AudioObjectPropertyAddress address=Address(Properties[n]);
			AudioObjectRemovePropertyListener(m_Device,&address,DeviceChanged,this);
		}

		AudioUnitUninitialize(m_Unit);
		AudioComponentInstanceDispose(m_Unit);
		m_Unit=NULL;
	}

	m_Listeners=0;
	m_Started=false;
	__sync_lock_test_and_set(&m_Attached,0);
	m_Playback=NULL;
	m_ProcessFrames=0;
	m_Capture.clear();
	if (notify && m_Run) m_Run(m_Context,0);

}

void CoreAudioClient::Failed()
{
	if (__sync_lock_test_and_set(&m_Attached,0) && m_Run) m_Run(m_Context,0);

}

OSStatus CoreAudioClient::DeviceChanged(AudioObjectID, UInt32 count, const AudioObjectPropertyAddress *addresses, void *context)
{
	CoreAudioClient *client=static_cast<CoreAudioClient *>(context);
	for (UInt32 n=0; n<count; ++n)
	{
		if (addresses[n].mSelector==kAudioDevicePropertyDeviceIsAlive)
		{
			UInt32 alive=0, size=sizeof(alive);
			if (AudioObjectGetPropertyData(client->m_Device,&addresses[n],0,NULL,&size,&alive)!=noErr || !alive)
				client->Failed();

		}

		else client->RefreshFormat();
	}

	return noErr;
}

OSStatus CoreAudioClient::Process(void *context, AudioUnitRenderActionFlags *flags,
	const AudioTimeStamp *time, UInt32, UInt32 frames, AudioBufferList *output)
{
	CoreAudioClient *client=static_cast<CoreAudioClient *>(context);
	if (output)
		for (UInt32 n=0; n<output->mNumberBuffers; ++n)
			if (output->mBuffers[n].mData) memset(output->mBuffers[n].mData,0,output->mBuffers[n].mDataByteSize);

	if (!client->IsAttached()) return noErr;

	__sync_lock_test_and_set(&client->m_Frames,frames);
	if (frames>client->m_Capacity)
	{
		client->Failed();
		return noErr;
	}

	if (client->m_Inputs)
	{
		AudioBufferList input;
		input.mNumberBuffers=1;
		input.mBuffers[0].mNumberChannels=client->m_Inputs;
		input.mBuffers[0].mDataByteSize=frames*client->m_Inputs*sizeof(float);
		input.mBuffers[0].mData=&client->m_Capture[0];
		if (AudioUnitRender(client->m_Unit,flags,time,1,frames,&input)!=noErr)
		{
			client->Failed();
			return noErr;
		}

	}

	client->m_Playback=output;
	client->m_ProcessFrames=frames;
	if (client->m_Run) client->m_Run(client->m_Context,frames);

	client->m_ProcessFrames=0;
	client->m_Playback=NULL;
	return noErr;
}

bool CoreAudioClient::Read(float *interleaved, unsigned int frames)
{
	if (!interleaved || !frames || frames!=m_ProcessFrames || !m_Inputs || !IsAttached()) return false;

	memcpy(interleaved,&m_Capture[0],frames*m_Inputs*sizeof(float));
	return true;
}

bool CoreAudioClient::Write(const float *interleaved, unsigned int frames)
{
	if (!interleaved || !frames || frames!=m_ProcessFrames || !m_Outputs || !IsAttached() ||
		!m_Playback || m_Playback->mNumberBuffers!=1) return false;
	AudioBuffer &buffer=m_Playback->mBuffers[0];
	unsigned bytes=frames*m_Outputs*sizeof(float);
	if (!buffer.mData || buffer.mDataByteSize<bytes) return false;

	memcpy(buffer.mData,interleaved,bytes);
	return true;
}
