// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPUMONI_IDENTITY_H
#define SPUMONI_IDENTITY_H

#include <string>
#include <vector>

namespace Spumoni
{
	class Folder;

	enum SaveMode
	{
		SaveReplace = 0,
		SaveNewBranch,
		SaveImportedBranch
	};

	struct Branch
	{
		std::string ID;
		std::string Name;
		std::string Kind;
		std::string ParentID;
		std::string ForkSaveID;
	};

	struct Identity
	{
		std::string PackageID;
		std::string ActiveBranchID;
		std::string AuthorUsername;
		// Opaque application metadata, interpreted only by its registered handler.
		std::string ApplicationMetadata;
		std::string ActiveBranchName;
		std::vector<Branch> Branches;
		void Clear()
		{
			PackageID.clear();
			ActiveBranchID.clear();
			AuthorUsername.clear();
			ApplicationMetadata.clear();
			ActiveBranchName.clear();
			Branches.clear();
		}

		Branch *Find(const std::string &id);
		Branch *FindByName(const std::string &name);
		bool NameTaken(const std::string &name, const std::string &exceptID = std::string()) const;
		void EnsureActiveListed();
		void AdoptUnlisted(const std::string &id);
		bool ActivateNamed(const std::string &name, std::string &error);
		bool Fork(const std::string &name, std::string &error);
		std::string SuggestedName() const;
		void Begin();

		static std::string TrimName(const std::string &name);
	};

	bool IsUUID(const std::string &value);
	std::string GenerateUUID();

	struct SaveRequest
	{
		SaveMode Mode;
		std::string NewBranchName;
		std::string ExistingPackage;
		const Folder *Workspace;
		std::string ExistingWorkspace;
		/* The branch whose extras (anything beside the application's payload)
		   carry over to the active branch; set by the application. */
		std::string SourceBranchID;
		class Store *SharedStore;
		SaveRequest()
			: Mode(SaveReplace), Workspace(NULL), SharedStore(NULL)
		{
		}
	};
}

#endif
