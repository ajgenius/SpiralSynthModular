// Native callback timing and lifecycle without opening an audio device.
#include "PortAudioClient.h"
#include <cassert>
#include <cmath>
#include <cstring>
using namespace spiralcore;

static PaStreamCallback *callback;
static void *context;
static bool active, inCallback;
static PaStreamInfo info = {1, 0.01, 0.02, 48000};
static PaDeviceInfo device = {2, "test", 0, 2, 2, 0.01, 0.02, 0.1, 0.1, 48000};
extern "C"
{
PaError Pa_Initialize() { assert(!inCallback); return paNoError; }
PaError Pa_Terminate() { assert(!inCallback); return paNoError; }
const char *Pa_GetErrorText(PaError) { return "test"; }
PaDeviceIndex Pa_GetDefaultInputDevice() { return 0; }
PaDeviceIndex Pa_GetDefaultOutputDevice() { return 0; }
PaDeviceIndex Pa_GetDeviceCount() { return 1; }
const PaDeviceInfo *Pa_GetDeviceInfo(PaDeviceIndex) { return &device; }
PaError Pa_OpenStream(PaStream **stream, const PaStreamParameters *, const PaStreamParameters *,
	double, unsigned long, PaStreamFlags, PaStreamCallback *run, void *data)
{
	assert(!inCallback && run);
	callback = run;
	context = data;
	*stream = reinterpret_cast<PaStream *>(1);
	return paNoError;
}
const PaStreamInfo *Pa_GetStreamInfo(PaStream *) { assert(!inCallback); return &info; }
PaTime Pa_GetStreamTime(PaStream *) { assert(!inCallback); return AudioMonotonicTime() - 100; }
PaError Pa_SetStreamFinishedCallback(PaStream *, PaStreamFinishedCallback *) { return paNoError; }
PaError Pa_StartStream(PaStream *) { assert(!inCallback); active = true; return paNoError; }
PaError Pa_IsStreamActive(PaStream *) { return active; }
PaError Pa_AbortStream(PaStream *) { assert(!inCallback); active = false; return paNoError; }
PaError Pa_CloseStream(PaStream *) { assert(!inCallback && !active); return paNoError; }
}
static unsigned cycles;
static double firstTime;
static void Run(void *data, unsigned frames)
{
	PortAudioClient &client = *static_cast<PortAudioClient *>(data);
	assert(frames == 16);
	AudioCycleTiming timing;
	assert(client.GetCycleTiming(timing));
	assert(timing.Frame == cycles * frames && timing.Frames == frames);
	assert(fabs(timing.OutputTime - firstTime - cycles * frames / 48000.0) < 1e-5);
	assert(fabs(timing.OutputTime - timing.InputTime - 0.03) < 1e-6);
	float samples[32];
	assert(client.Read(samples, frames));
	for (unsigned n = 0; n < 32; ++n) assert(samples[n] == float(n));

	assert(client.Write(samples, frames));
	++cycles;
}
int main()
{
	PortAudioClient *client = PortAudioClient::Get();
	AudioClientOptions options;
	options.InChannels = options.OutChannels = 2;
	options.BufferSize = 16;
	assert(client->Attach("default", options));
	assert(client->IsCallbackDriven() && !active);
	client->SetCallback(Run, client);
	assert(client->Start() && active);
	firstTime = AudioMonotonicTime() + 0.02;
	for (unsigned n = 0; n < 100; ++n)
	{
		const double now = firstTime - 100 + n * 16 / 48000.0;
		PaStreamCallbackTimeInfo time = {now - 0.03, now - 0.02, now};
		float input[32], output[32];
		for (unsigned s = 0; s < 32; ++s) input[s] = float(s);

		inCallback = true;
		assert(callback(input, output, 16, &time, 0, context) == paContinue);
		inCallback = false;
		assert(memcmp(input, output, sizeof(input)) == 0);
	}

	AudioCycleTiming timing;
	assert(!client->GetCycleTiming(timing));
	client->Detach();
	client->Detach();
	assert(!client->IsAttached() && !active);
	PortAudioClient::PackUpAndGoHome();
}
