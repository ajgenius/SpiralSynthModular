// SPDX-License-Identifier: GPL-2.0-or-later
// Spiral::File::Project — the bare tip of an SSM project.
//
#ifndef SPIRAL_FILE_PROJECT_H
#define SPIRAL_FILE_PROJECT_H

#include "Project.h"
#include "SourcePart.h"

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

		class Project : public Spumoni::Project
		{
		public:
			explicit Project(const std::string &path);
			virtual ~Project();

			const Spumoni::SourcePart &Source() const { return *m_Source; }
			Spumoni::SourcePart &Source() { return *m_Source; }

			const Spumoni::SourcePart &Patch() const { return *m_Patch; }
			Spumoni::SourcePart &Patch() { return *m_Patch; }
			bool HasPatch() const { return m_Patch && !m_Patch->Empty(); }

			DocumentSection GetDocument() const;

			void SetDocument(const DocumentSection &document);

			bool BeginSidecars(std::string &error);
			std::string SidecarDirectory() const;

			// Extension (.ssmp, .tar, a directory) or a manifest inside.
			static bool PathLooksLikePackage(const std::string &path);

		protected:
			virtual bool HasContent() const { return (m_Source && !m_Source->Empty()) || HasPatch(); }
			virtual void OnReset();

		private:
			class SidecarPart;
			class LicensePart;
			Spumoni::SourcePart *m_Source;
			Spumoni::SourcePart *m_Patch;
			SidecarPart *m_Sidecars;
			LicensePart *m_License;
		};
	}
}

#endif
