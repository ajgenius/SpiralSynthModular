#include "CoreAudioClient.h"
#include "AudioBackend.h"
#include <algorithm>
#include <CoreAudio/HostTime.h>
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

	unsigned PropertyFrames(AudioObjectID object, AudioObjectPropertySelector selector, AudioObjectPropertyScope scope)
	{
		AudioObjectPropertyAddress address = {selector, scope, kAudioObjectPropertyElementMaster};
		UInt32 value = 0, size = sizeof(value);
		AudioObjectGetPropertyData(object, &address, 0, NULL, &size, &value);
		return value;
	}

	void HardwareLatency(AudioDeviceID device, bool input, unsigned result[2])
	{
		const AudioObjectPropertyScope scope = input ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
		const unsigned deviceLatency = PropertyFrames(device, kAudioDevicePropertyLatency, scope);
		result[0] = result[1] = deviceLatency;
		AudioObjectPropertyAddress address = {kAudioDevicePropertyStreams, scope, kAudioObjectPropertyElementMaster};
		UInt32 size = 0;
		if (AudioObjectGetPropertyDataSize(device, &address, 0, NULL, &size) != noErr || !size) return;

		std::vector<AudioStreamID> streams(size / sizeof(AudioStreamID));
		if (AudioObjectGetPropertyData(device, &address, 0, NULL, &size, &streams[0]) != noErr) return;

		for (unsigned n = 0; n < streams.size(); ++n)
		{
			const unsigned first = PropertyFrames(streams[n], kAudioStreamPropertyStartingChannel, kAudioObjectPropertyScopeGlobal);
			AudioStreamBasicDescription format;
			address = Address(kAudioStreamPropertyVirtualFormat);
			size = sizeof(format);
			if (!first || AudioObjectGetPropertyData(streams[n], &address, 0, NULL, &size, &format) != noErr) continue;

			const unsigned latency = deviceLatency + PropertyFrames(streams[n], kAudioStreamPropertyLatency, kAudioObjectPropertyScopeGlobal);
			for (unsigned c = first - 1; c < 2 && c < first - 1 + format.mChannelsPerFrame; ++c)
				result[c] = latency;

		}

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
	m_Started(false), m_Listeners(0), m_InputSoftware(0), m_OutputSoftware(0),
	m_HostTime(0), m_Frame(0), m_Playback(NULL), m_Run(NULL), m_Context(NULL)
{
	m_InputHardware[0] = m_InputHardware[1] = 0;
	m_OutputHardware[0] = m_OutputHardware[1] = 0;
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

	unsigned input[2], output[2];
	HardwareLatency(m_Device, true, input);
	HardwareLatency(m_Device, false, output);
	for (unsigned c = 0; c < 2; ++c)
	{
		__sync_lock_test_and_set(&m_InputHardware[c], input[c]);
		__sync_lock_test_and_set(&m_OutputHardware[c], output[c]);
	}

	__sync_lock_test_and_set(&m_InputSoftware, frames +
		PropertyFrames(m_Device, kAudioDevicePropertySafetyOffset, kAudioDevicePropertyScopeInput) +
		PropertyFrames(m_Device, kAudioDevicePropertyLatency, kAudioDevicePropertyScopeInput));
	__sync_lock_test_and_set(&m_OutputSoftware, frames +
		PropertyFrames(m_Device, kAudioDevicePropertySafetyOffset, kAudioDevicePropertyScopeOutput) +
		PropertyFrames(m_Device, kAudioDevicePropertyLatency, kAudioDevicePropertyScopeOutput));
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

	// Optional properties vary across drivers. Register only supported scopes;
	// all queries and allocation happen off the render callback.
	const AudioObjectPropertySelector timingProperties[] = {
		kAudioDevicePropertyLatency, kAudioDevicePropertySafetyOffset, kAudioDevicePropertyStreams
	};
	for (unsigned direction = 0; direction < 2; ++direction)
		for (unsigned n = 0; n < 3; ++n)
		{
			AudioObjectPropertyAddress address = Address(timingProperties[n]);
			address.mScope = direction ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
			if (AudioObjectHasProperty(m_Device, &address) &&
				AudioObjectAddPropertyListener(m_Device, &address, DeviceChanged, this) == noErr)
				m_TimingListeners.push_back(address);

		}

	AudioMonotonicTime();
	AudioGetCurrentHostTime();
	m_Frame = 0;
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

		for (unsigned n = 0; n < m_TimingListeners.size(); ++n)
			AudioObjectRemovePropertyListener(m_Device, &m_TimingListeners[n], DeviceChanged, this);

		m_TimingListeners.clear();
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

	client->m_HostTime = AudioConvertHostTimeToNanos(time->mHostTime) * 1e-9;
	client->m_Timing.Frame = (time->mFlags & kAudioTimeStampSampleTimeValid) && time->mSampleTime >= 0
		? uint64_t(time->mSampleTime) : client->m_Frame;
	client->m_Timing.Frames = frames;
	client->m_Timing.SampleRate = client->GetSampleRate();
	client->m_Timing.CallbackTime = AudioMonotonicTime();
	client->m_Timing.Valid = (time->mFlags & kAudioTimeStampHostTimeValid) && client->GetSampleRate();
	client->m_Playback=output;
	client->m_ProcessFrames=frames;
	client->m_Timing.InputTime = client->GetChannelTime(true, 0);
	client->m_Timing.OutputTime = client->GetChannelTime(false, 0);
	if (client->m_Run) client->m_Run(client->m_Context,frames);

	client->m_Frame += frames;

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


bool CoreAudioClient::GetCycleTiming(AudioCycleTiming &timing) const
{
	if (!m_ProcessFrames || !m_Timing.Valid) return false;

	timing = m_Timing;
	return true;
}

double CoreAudioClient::GetChannelTime(bool input, unsigned channel) const
{
	if (channel >= 2 || !m_ProcessFrames || !GetSampleRate()) return 0;

	const double rate = GetSampleRate();
	if (!input)
		return m_HostTime + __sync_fetch_and_add(&m_OutputHardware[channel], 0) / rate;

	// HAL's output timestamp already includes its software buffer and safety
	// offset. Duplex input is older by both sides' software pipelines.
	const unsigned software = m_Outputs ? __sync_fetch_and_add(&m_InputSoftware, 0) +
		__sync_fetch_and_add(&m_OutputSoftware, 0) : 0;
	return m_HostTime - (software + __sync_fetch_and_add(&m_InputHardware[channel], 0)) / rate;
}

double CoreAudioClient::GetInputLatency() const
{
	if (!GetSampleRate()) return 0;

	return double(__sync_fetch_and_add(&m_InputSoftware, 0) +
		std::max(__sync_fetch_and_add(&m_InputHardware[0], 0), __sync_fetch_and_add(&m_InputHardware[1], 0))) / GetSampleRate();
}

double CoreAudioClient::GetOutputLatency() const
{
	if (!GetSampleRate()) return 0;

	return double(__sync_fetch_and_add(&m_OutputSoftware, 0) +
		std::max(__sync_fetch_and_add(&m_OutputHardware[0], 0), __sync_fetch_and_add(&m_OutputHardware[1], 0))) / GetSampleRate();
}

// * Backend module entry

static void *CreateCoreAudio(void *) { return static_cast<AudioClient *>(new CoreAudioClient); }
static void DestroyCoreAudio(void *client) { delete static_cast<AudioClient *>(client); }
static const BackendDescriptor CoreAudioBackend = { SPIRAL_AUDIO_PLUGIN_ABI, "audio", "coreaudio", CreateCoreAudio, DestroyCoreAudio };

extern "C" const BackendDescriptor *SpiralPlugin_GetAudioBackend() { return &CoreAudioBackend; }
