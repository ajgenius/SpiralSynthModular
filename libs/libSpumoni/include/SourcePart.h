// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::SourcePart — a file kept whole: the one a project was made from.
// An application gives it a fixed name in the branch ("source.ssm") and
// sets the bytes when it imports. A branch saved without one has no such
// file, and one opened with it has the bytes back. Optional.
#ifndef SPUMONI_SOURCE_PART_H
#define SPUMONI_SOURCE_PART_H

#include "Project.h"

namespace Spumoni
{
	class SourcePart : public Part
	{
	public:
		explicit SourcePart(const std::string &name) : m_Name(name) {}

		/* The file, from the import. Clear forgets it. */
		void Set(const std::string &bytes);
		void Clear() { m_Live.clear(); m_Staged.clear(); }
		bool Empty() const { return m_Live.empty(); }
		const std::string &Bytes() const { return m_Live; }

		virtual std::string Name() const { return m_Name; }
		virtual bool Required() const { return false; }

		virtual bool Load(Folder &folder, const std::string &branchRoot,
			const std::string &packagePath, std::string &error);
		virtual void Commit() { m_Live.swap(m_Staged); m_Staged.clear(); }
		virtual void Discard() { m_Staged.clear(); }
		virtual bool Store(Container::Writer &writer, const std::string &branchRoot,
			class Store *, std::string &error);

	private:
		std::string m_Name;
		std::string m_Live;
		std::string m_Staged;
	};
}

#endif
