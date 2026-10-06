// SPDX-License-Identifier: GPL-2.0-or-later
#include "AudioTiming.h"
#include <cmath>
#ifdef __APPLE__
#include <mach/mach_time.h>
#include <pthread.h>
#else
#include <time.h>
#endif

namespace spiralcore
{
#ifdef __APPLE__
namespace
{
	pthread_once_t TimebaseOnce = PTHREAD_ONCE_INIT;
	double TimebaseSeconds;
	void InitializeTimebase()
	{
		mach_timebase_info_data_t info;
		mach_timebase_info(&info);
		TimebaseSeconds = double(info.numer) / info.denom / 1e9;
	}
}
#endif

double AudioMonotonicTime()
{
#ifdef __APPLE__
	pthread_once(&TimebaseOnce, InitializeTimebase);
	return mach_absolute_time() * TimebaseSeconds;
#else
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec + now.tv_nsec / 1e9;
#endif
}

AudioTimingMailbox::AudioTimingMailbox() : m_Back(0), m_Front(1), m_Middle(2) {}

void AudioTimingMailbox::Publish(const AudioCycleTiming &value)
{
	m_Slots[m_Back] = value;
	__sync_synchronize();
	m_Back = __sync_lock_test_and_set(&m_Middle, m_Back | 4) & 3;
	__sync_synchronize();
}

bool AudioTimingMailbox::Read(AudioCycleTiming &value)
{
	if (!(__sync_fetch_and_add(&m_Middle, 0) & 4)) return false;

	m_Front = __sync_lock_test_and_set(&m_Middle, m_Front) & 3;
	__sync_synchronize();
	value = m_Slots[m_Front];
	return true;
}

AudioRateEstimator::AudioRateEstimator() { Reset(); }

void AudioRateEstimator::Reset()
{
	m_Ready = false;
	m_Frame = 0;
	m_Time = m_Step = 0;
}

double AudioRateEstimator::Observe(uint64_t frame, double time, double nominalRate)
{
	if (!(nominalRate > 0) || !(time >= 0)) return 0;

	const double nominalStep = 1 / nominalRate;
	if (!m_Ready || frame <= m_Frame || time <= m_Time || time - m_Time > 1)
	{
		m_Step = nominalStep;
	}
	else
	{
		const double observed = (time - m_Time) / double(frame - m_Frame);
		if (std::fabs(observed / nominalStep - 1) < 0.01)
			m_Step += (observed - m_Step) * 0.02;
		else
			m_Step = nominalStep;
	}

	m_Ready = true;
	m_Frame = frame;
	m_Time = time;
	return m_Step;
}
}
