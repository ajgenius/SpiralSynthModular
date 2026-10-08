// SPDX-License-Identifier: GPL-2.0-or-later
// Spiral::File::Project — the bare tip of an SSM project.
//
// Two parts: patch.spiral.legacy.ssm and patch.spiral.legacy.ssm_files/.
// The patch keeps its own version header. Opening a package reads those
// bytes and the existing loader streams them in. No patch JSON, no
// schema, no property stack. Draft, not ABI stable.
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
		// What a project says about itself. Spumoni keeps the metadata
		// file's text opaque because only this handler knows its shape,
		// so the shape lives here, beside the code that writes it.
		//
		// Field names are the private tree's (Spiral::DocumentSection in
		// libSSMCore/Description.h), so the two can meet without a
		// translation step when the scope tree arrives here.
		struct Credit
		{
			std::string Name;
			std::string Role;
		};

		struct RightsSection
		{
			std::string Copyright;
			std::string License;
		};

		struct DocumentSection
		{
			std::string Title;
			std::string Description;
			// Who saved it and when, as the file recorded them. Written by
			// the save path, not stated by hand.
			std::string SavedBy;
			std::string CreatedAt;
			std::string SavedAt;
			std::vector<Credit> Credits;
			RightsSection Rights;

			// Nothing claimed: the project says nothing about itself, so
			// there is no document to show rather than an empty one.
			bool Empty() const
			{
				return Title.empty() && Description.empty()
					&& SavedBy.empty() && CreatedAt.empty()
					&& SavedAt.empty() && Credits.empty()
					&& Rights.Copyright.empty() && Rights.License.empty();
			}
		};

		class Project : public Spumoni::Project
		{
		public:
			explicit Project(const std::string &path);
			virtual ~Project();

			const Spumoni::SourcePart &Source() const { return *m_Source; }
			Spumoni::SourcePart &Source() { return *m_Source; }

			// The document as the file states it, read back out of the
			// metadata text the load kept. Empty for a project that claims
			// nothing, and for anything that is not a package at all.
			DocumentSection GetDocument() const;

			// A fresh directory for the plugins to populate on save; cleared each
			// time so removed samples cannot survive in a replacement branch.
			bool BeginSidecars(std::string &error);
			std::string SidecarDirectory() const;

			// Extension (.ssmp, .tar, a directory) or a manifest inside.
			static bool PathLooksLikePackage(const std::string &path);

		protected:
			virtual bool HasContent() const { return m_Source && !m_Source->Empty(); }
			virtual void OnReset();

		private:
			class SidecarPart;
			Spumoni::SourcePart *m_Source;
			SidecarPart *m_Sidecars;
		};
	}
}

#endif
