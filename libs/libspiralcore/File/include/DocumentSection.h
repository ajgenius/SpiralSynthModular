// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_DOCUMENT_SECTION_H
#define SPIRAL_DOCUMENT_SECTION_H
#include <string>
#include <vector>

namespace Spiral
{
	namespace File
	{
		struct Credit
		{
			std::string Name;
			std::string Role;
		};

		struct RightsSection
		{
			std::string Copyright;
			std::string License;
			bool BundleText;
			std::string LicenseFile;
			RightsSection() : BundleText(false) {}
			bool Empty() const
			{
				return Copyright.empty() && License.empty()
					&& !BundleText && LicenseFile.empty();
			}
		};

		struct DocumentSection
		{
			std::string Title;
			std::string Description;
			std::string SavedBy;
			std::string CreatedAt;
			std::string SavedAt;
			std::vector<Credit> Credits;
			RightsSection Rights;

			bool Empty() const
			{
				return Title.empty() && Description.empty()
					&& SavedBy.empty() && CreatedAt.empty()
					&& SavedAt.empty() && Credits.empty()
					&& Rights.Empty();
			}
		};

	}
}
#endif
