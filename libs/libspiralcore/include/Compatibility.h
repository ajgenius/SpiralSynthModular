// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SSM_COMPATIBILITY_H
#define SSM_COMPATIBILITY_H
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#ifdef __APPLE__
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

namespace SSMCompat
{
	template <class T> std::string ToString(T value)
	{
		std::ostringstream text;
		text.imbue(std::locale::classic());
		text << value;
		return text.str();
	}

	inline bool IsFinite(double value)
	{
		return value >= -std::numeric_limits<double>::max() && value <= std::numeric_limits<double>::max();
	}

	inline double MonotonicMilliseconds()
	{
#ifdef __APPLE__
		mach_timebase_info_data_t scale;
		mach_timebase_info(&scale);
		return double(mach_absolute_time()) * scale.numer / scale.denom / 1000000.0;
#else
		struct timespec now;
		if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
			throw std::runtime_error("Cannot read the monotonic clock");
		return double(now.tv_sec) * 1000.0 + now.tv_nsec / 1000000.0;
#endif
	}
}
#endif
