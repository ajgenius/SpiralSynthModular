// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRALCORE_WASAPI_CLIENT_H
#define SPIRALCORE_WASAPI_CLIENT_H

#include "AudioClient.h"

namespace Spiral
{
namespace Core
{
// Shared-mode float output. Control methods are serialized by the host;
// Write and timing queries belong to the installed callback only.
class WASAPIClient : public AudioClient
{
public:
	WASAPIClient();
	virtual ~WASAPIClient();
	bool Attach(const std::string &device, const AudioClientOptions &options);
	void Detach();
	bool IsAttached() const;
	bool IsCallbackDriven() const { return true; }
	void SetCallback(void (*run)(void *, unsigned), void *context);
	bool Start();
	unsigned long GetBufferSize() const;
	unsigned long GetSampleRate() const;
	double GetOutputLatency() const;
	bool GetCycleTiming(AudioCycleTiming &timing) const;
	bool Write(const float *interleaved, unsigned frames);
	bool Read(float *, unsigned) { return false; }
	long LastError() const;

private:
	struct Platform;
	Platform *m_Platform;
	WASAPIClient(const WASAPIClient &);
	WASAPIClient &operator=(const WASAPIClient &);
};
}
}
#endif
