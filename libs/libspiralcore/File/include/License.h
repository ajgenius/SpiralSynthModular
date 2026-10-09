// SPDX-License-Identifier: GPL-2.0-or-later
// The licences a project may claim, by SPDX identifier, and the full text
// of those this build can write into a package.
//
// Same shape as the private tree's (Spiral::LicensePreset in
// libSSMCore/Description.h), so a panel written against one works against
// the other. Toolkit-free: the table is data, the GUI only lists it.
#ifndef SPIRAL_FILE_LICENSE_H
#define SPIRAL_FILE_LICENSE_H

#include <string>

namespace Spiral
{
	namespace File
	{
		// Identifiers offered in display order. Bundles is true when
		// LicenseFullText has the complete text to put in the archive;
		// the others are still selectable, they just cannot be inlined.
		struct LicensePreset
		{
			const char *Id;
			bool Bundles;
		};

		size_t LicensePresetCount();
		LicensePreset LicensePresetAt(size_t index);

		// The complete text for an identifier, or empty when this build
		// does not carry one.
		std::string LicenseFullText(const std::string &spdx);
	}
}

#endif
