// SPDX-License-Identifier: GPL-2.0-or-later
#include "SourcePart.h"
#include "Folder.h"

using namespace std;

namespace Spumoni
{
	void SourcePart::Set(const string &bytes)
	{
		m_Live = bytes;
		m_Staged.clear();
	}

	// Load is only asked when the file is there (the part is optional);
	// a branch without one leaves the staging empty, and Commit makes
	// that live: opening a branch with no source forgets any earlier one.
	bool SourcePart::Load(Folder &folder, const string &branchRoot, const string &, string &error)
	{
		return folder.Read(branchRoot + m_Name, m_Staged, error);
	}

	bool SourcePart::Store(Container::Writer &writer, const string &branchRoot, string &error)
	{
		if (m_Live.empty()) return true;
		return writer.AddMemory(branchRoot + m_Name, m_Live, error);
	}
}
