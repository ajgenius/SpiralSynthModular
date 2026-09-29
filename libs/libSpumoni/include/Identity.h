// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Identity — what a package says about itself: its UUID, its save
// points (branches) with their UUIDs, names and lineage, which one is
// active, and who saved it. With the bookkeeping an application does on
// them: listing the active branch, forking a named one, activating a named
// one, suggesting a free name. No file knowledge: Package only reads and
// writes an identity as it is given. SaveRequest is what a write is asked.
#ifndef SPUMONI_IDENTITY_H
#define SPUMONI_IDENTITY_H

#include <string>
#include <vector>

namespace Spumoni
{
	class Folder;

	enum SaveMode
	{
		/* Overwrite the named branch's working files (active moves to that
		   branch). Other branch folders are copied forward unchanged. */
		SaveReplace = 0,
		/* Keep prior branches and write the current patch as a new named
		   branch in the same package. The filename does not change. */
		SaveNewBranch
	};

	struct Branch
	{
		std::string ID;
		std::string Name;
		std::string Kind;
		std::string ParentID;
		std::string ForkSaveID;
	};

	/* Stable package and active-branch identity retained across load/save. */
	struct Identity
	{
		std::string PackageID;
		std::string ActiveBranchID;
		std::string AuthorUsername;
		std::string ActiveBranchName;
		std::vector<Branch> Branches;
		void Clear()
		{
			PackageID.clear();
			ActiveBranchID.clear();
			AuthorUsername.clear();
			ActiveBranchName.clear();
			Branches.clear();
		}

		Branch *Find(const std::string &id);
		Branch *FindByName(const std::string &name);
		bool NameTaken(const std::string &name, const std::string &exceptID = std::string()) const;
		/* Make sure the active branch is in Branches (with its current name). */
		void EnsureActiveListed();
		/* Add a branch folder the manifest did not list (found in a package). */
		void AdoptUnlisted(const std::string &id);
		/* replaceIfExists: move active TO the named branch (its files are then
		   overwritten by the write). */
		bool ActivateNamed(const std::string &name, std::string &error);
		/* A new branch under the active one becomes active. With no active
		   branch yet the name simply names the first save. */
		bool Fork(const std::string &name, std::string &error);
		/* "<active> 2", "<active> 3", … — the first name not taken. */
		std::string SuggestedName() const;
		/* First save: fresh package and branch ids. */
		void Begin();

		static std::string TrimName(const std::string &name);
	};

	/* Branch and package ids. */
	bool IsUUID(const std::string &value);
	std::string GenerateUUID();

	/* What the application asks of a write. It resolves Mode and NewBranchName
	   onto the identity; Package only sees the sources to preserve from. */
	struct SaveRequest
	{
		SaveMode Mode;
		std::string NewBranchName;
		/* Same-package save point only: the prior package to copy other
		   branch folders (and anything else in them) into the replacement.
		   Not used for SaveAs-to-a-different-filename (fresh package). */
		std::string ExistingPackage;
		/* Live extract for a same-package save point when a workspace is open:
		   the Folder itself (any kind, not owned), or a directory path. */
		const Folder *Workspace;
		std::string ExistingWorkspace;
		/* The branch whose extras (anything beside the patch, contract and
		   assets) carry over to the active branch; set by the application. */
		std::string SourceBranchID;
		SaveRequest()
			: Mode(SaveReplace), Workspace(NULL)
		{
		}
	};
}

#endif
