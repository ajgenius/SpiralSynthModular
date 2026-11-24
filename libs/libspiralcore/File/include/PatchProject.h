// SPDX-License-Identifier: GPL-2.0-or-later
// Spiral::File::Project — the bare tip of an SSM project.
//
#ifndef SPIRAL_FILE_PROJECT_H
#define SPIRAL_FILE_PROJECT_H

#include "Project.h"
#include "DocumentSection.h"
#include "SourcePart.h"

#include <string>
#include <vector>

namespace Spiral
{
	namespace File
	{
		class Project : public Spumoni::Project
		{
		public:
			explicit Project(const std::string &path);
			virtual ~Project();

			const Spumoni::SourcePart &Source() const { return *m_Source; }
			Spumoni::SourcePart &Source() { return *m_Source; }

			const Spumoni::SourcePart &Patch() const { return *m_Patch; }
			Spumoni::SourcePart &Patch() { return *m_Patch; }
			const Spumoni::SourcePart &Schema() const { return *m_Schema; }
			bool HasPatch() const { return m_Patch && !m_Patch->Empty(); }

			DocumentSection GetDocument() const;
			static DocumentSection ReadDocument(const Spumoni::JSON &root);
			static void WriteDocument(Spumoni::JSON &root, const DocumentSection &document);

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
			Spumoni::SourcePart *m_Schema;
			SidecarPart *m_Sidecars;
			LicensePart *m_License;
		};
	}
}

#endif
