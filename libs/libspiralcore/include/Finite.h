#ifndef SPIRALCORE_FINITE_H
#define SPIRALCORE_FINITE_H

#include <limits>

namespace spiralcore
{
// C++98-compatible check, including platforms without std::isfinite.
inline bool IsFinite(float value)
{
	const float limit = std::numeric_limits<float>::max();
	return value == value && value >= -limit && value <= limit;
}
}

#endif
