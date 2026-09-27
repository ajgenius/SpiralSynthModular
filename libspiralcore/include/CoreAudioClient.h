// Native macOS device callbacks through the shared audio-client interface.
#ifndef SPIRALCORE_CORE_AUDIO_CLIENT
#define SPIRALCORE_CORE_AUDIO_CLIENT

#include "AudioClient.h"
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>
#include <vector>

namespace spiralcore
{
class CoreAudioClient : public AudioClient
{
public:
	CoreAudioClient();
	virtual ~CoreAudioClient();
	bool Attach(const std::string &device, const AudioClientOptions &options);
	void Detach();
	bool IsAttached() const { return __sync_fetch_and_add(&m_Attached, 0)!=0; }

	bool IsCallbackDriven() const { return true; }

	void SetCallback(void (*run)(void *, unsigned int), void *context);
	bool Start();

	unsigned long GetBufferSize() const { return __sync_fetch_and_add(&m_Frames, 0); }

	unsigned long GetSampleRate() const { return __sync_fetch_and_add(&m_Rate, 0); }

	bool Read(float *interleaved, unsigned int frames);

	bool Write(const float *interleaved, unsigned int frames);

private:
	static OSStatus Process(void *, AudioUnitRenderActionFlags *, const AudioTimeStamp *, UInt32, UInt32, AudioBufferList *);
	static OSStatus DeviceChanged(AudioObjectID, UInt32, const AudioObjectPropertyAddress *, void *);
	bool SetFormat(AudioUnitScope scope, AudioUnitElement element, unsigned channels);
	void RefreshFormat();
	void Failed();

	AudioUnit m_Unit;
	AudioDeviceID m_Device;

	unsigned m_Inputs, m_Outputs, m_Capacity, m_ProcessFrames;
	mutable unsigned m_Frames, m_Rate;
	mutable int m_Attached;
	bool m_Started;

	unsigned m_Listeners;

	std::vector<float> m_Capture;
	AudioBufferList *m_Playback;
	void (*m_Run)(void *, unsigned int);
	void *m_Context;
	CoreAudioClient(const CoreAudioClient &);
	CoreAudioClient &operator=(const CoreAudioClient &);
};

}

#endif
