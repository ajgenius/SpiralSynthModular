// SPDX-License-Identifier: GPL-2.0-or-later
#include "License.h"

#include <cstddef>

namespace
{
#include "LicenseTexts.inc"

	struct LicenseRow
	{
		const char *id;
		const char *body;
	};

	// Dropdown order. A NULL body is offered as an identifier only: the
	// panel will not claim to have inlined a text it does not have.
	const char kReserved[] =
		"All rights reserved.\n\n"
		"No licence is granted. Copying, modification, and redistribution "
		"require permission from the copyright holder, except for uses that "
		"the law already allows.\n";

	const LicenseRow kLicenses[] = {
		{"GPL-2.0-or-later", kGpl2},
		{"GPL-2.0-only", kGpl2},
		{"GPL-3.0-or-later", NULL},
		{"LGPL-2.1-or-later", NULL},
		{"MIT", kMit},
		{"BSD-2-Clause", kBsd2},
		{"BSD-3-Clause", kBsd3},
		{"CC-BY-4.0", NULL},
		{"CC-BY-SA-4.0", NULL},
		{"CC-BY-SA-1.0", kCcBySa10},
		{"CC0-1.0", kCc0},
		{"All-rights-reserved", kReserved}
	};
	const size_t kLicenseCount = sizeof(kLicenses) / sizeof(kLicenses[0]);
}

size_t Spiral::File::LicensePresetCount()
{
	return kLicenseCount;
}

Spiral::File::LicensePreset Spiral::File::LicensePresetAt(size_t index)
{
	LicensePreset row;

	row.Id = index < kLicenseCount ? kLicenses[index].id : "";
	row.Bundles = index < kLicenseCount && kLicenses[index].body != NULL;

	return row;
}

std::string Spiral::File::LicenseFullText(const std::string &spdx)
{
	for (size_t i = 0; i < kLicenseCount; ++i)
		if (spdx == kLicenses[i].id && kLicenses[i].body)
			return std::string(kLicenses[i].body);

	return std::string();
}
