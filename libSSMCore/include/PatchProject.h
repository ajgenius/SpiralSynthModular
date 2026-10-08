// SPDX-License-Identifier: GPL-2.0-or-later
// Spiral::File::Project — the bare tip of an SSM project.
//
// One part: source.ssm, the patch kept whole. Opening a package reads
// those bytes and the existing loader streams them in. No patch JSON,
// no schema, no property stack. Draft, not ABI stable.
#ifndef SPIRAL_FILE_PROJECT_H
#define SPIRAL_FILE_PROJECT_H

#include "Project.h"
#include "SourcePart.h"

#include <string>

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

			// Extension (.ssmp, .tar, a directory) or a manifest inside.
			static bool PathLooksLikePackage(const std::string &path);

		protected:
			virtual bool HasContent() const { return m_Source && !m_Source->Empty(); }
			virtual void OnReset() { if (m_Source) m_Source->Clear(); }

		private:
			Spumoni::SourcePart *m_Source;
		};
	}
}

#endif
