// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_FILE_LICENSE_H
#define SPIRAL_FILE_LICENSE_H

#include <string>

namespace Spiral
{
	namespace File
	{
		struct LicensePreset
		{
			const char *Id;
			bool Bundles;
		};

		size_t LicensePresetCount();
		LicensePreset LicensePresetAt(size_t index);

		std::string LicenseFullText(const std::string &spdx);
	}
}

#endif
