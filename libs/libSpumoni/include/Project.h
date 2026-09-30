// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Project — an open package: its identity, its working folder and
// the parts the application keeps in a branch, with the operations on them.
//
// A branch is a set of Parts, each a role the application fills: a data
// file, an optional schema beside it, whatever else it wants versioned per
// save point. The manifest's application section is the third role
// (Package::Application). Project owns none of the content: it owns the
// identity, the source path and the workspace, and it knows the save
// surface every layered package has:
//   Open / OpenSavePoint  — unpack into a workspace, load the parts
//   SaveAs                — a different file: a fresh package, one branch;
//                           the same file: replace the branch named after it
//   CreateSavePoint       — this state as a named branch, new or replaced
//   UpdateManifest        — the manifest only
// An application derives, adds its parts and its content, and tells
// Project what a Format is: manifest name, application section, extensions.
#ifndef SPUMONI_PROJECT_H
#define SPUMONI_PROJECT_H

#include "Folder.h"
#include "Identity.h"
#include "Package.h"

#include <string>
#include <vector>

namespace Spumoni
{

	// * One file role inside a branch. Load reads into the part's own staging
	//   (nothing live changes until Commit, so a failed open leaves the
	//   project as it was); Store writes the live state into a package.
	class Part
	{
	public:
		virtual ~Part() {}

		virtual std::string Name() const = 0;
		virtual std::string LegacyName() const { return std::string(); }
		virtual bool Required() const { return true; }

		// The folder is not const: a part may ask it for real paths.
		virtual bool Load(Folder &folder, const std::string &branchRoot,
			const std::string &packagePath, std::string &error) = 0;
		virtual void Commit() = 0;
		virtual void Discard() = 0;
		// shared is the workspace's Store when it has one (Ver 2): a part may
		// put its bundled files there and write their addresses instead of
		// copying them under the branch. NULL keeps everything in the branch.
		virtual bool Store(Container::Writer &writer, const std::string &branchRoot,
			class Store *shared, std::string &error) = 0;
	};

	// * What makes a package this application's: shared by all its projects.
	struct Format
	{
		std::string ManifestName;
		std::string MetadataName;
		std::string MetadataKey;
		std::string LegacyManifestName;
		const Package::Application *Application;
		std::vector<std::string> Extensions;	// ".ssmp", lower case, with the dot
		std::string WorkPrefix;
		unsigned long long MemoryLimit;		// a larger package works on disk
		Format() : Application(NULL), MemoryLimit(256ULL * 1024 * 1024) {}
	};

	class Project
	{
	public:
		virtual ~Project();

		// * Clear content and identity, adopt a path (empty = new), and give
		//   the project a fresh working folder: a new project has no backing
		//   file yet, but it has a workspace laid out as a package, with its
		//   identity begun, from the start. The application's content is
		//   cleared through OnReset.
		void Reset(const std::string &path = std::string());

		// * Unpack the package at SourcePath and load the parts (an empty id
		//   selects the active branch). The application's Open decides what
		//   else a path can be.
		bool OpenPackage(const std::string &branchId, std::string &error);
		bool OpenSavePoint(const std::string &branchId, std::string &error);
		bool SwitchBranch(const std::string &branchId, std::string &error);

		bool SaveAs(const std::string &path, std::string &error);
		bool CreateSavePoint(const std::string &branchName, bool replaceIfExists, std::string &error, bool independent = false);
		bool UpdateManifest(std::string &error);

		// * Write the parts into a package at `path` with the given request,
		//   deciding the identity: which branch, what name, what is listed.
		bool WritePackage(const std::string &path, SaveRequest request, std::string &error);

		void AdoptIdentity(const Identity &identity) { m_Identity = identity; }
		const Identity &GetIdentity() const { return m_Identity; }
		Identity &GetIdentity() { return m_Identity; }
		const std::string &SourcePath() const { return m_SourcePath; }

		// * The working folder: the unpacked package while one is open
		//   (memory unless large), refreshed from the file after every save
		//   so it always mirrors it; a laid-out empty package for a new
		//   project. NULL only after ReleaseWorkspace, until the next open,
		//   save or Reset.
		const Folder *Workspace() const { return m_Workspace; }
		Folder *Workspace() { return m_Workspace; }
		Folder *ReleaseWorkspace();
		static void DestroyWorkspace(Folder *&workspace);

		// * A saved package to write save points into?
		bool HasPackagePath() const { return LooksLikePackage(m_SourcePath); }

		// * Path judgements for this format.
		bool LooksLikePackage(const std::string &path) const;
		bool IsPackage(const std::string &path) const;	// by content, manifest present
		std::string BranchNameFromPath(const std::string &path) const;
		static bool SameFile(const std::string &a, const std::string &b);
		static std::string SystemUsername();
		static std::string SuggestedSavePointName(const Identity &identity) { return identity.SuggestedName(); }

		const Format &GetFormat() const { return m_Format; }
		std::string PackageWord() const;
		Package::Layout Layout() const;
		Folder *NewWorkspaceFor(const std::string &path) const;

	protected:
		explicit Project(const Format &format);

		// * The parts, in branch order; the application owns them.
		void AddPart(Part &part);

		// * Is there anything to save? Called before every write.
		virtual bool HasContent() const = 0;
		// * Clear the application's content on Reset.
		virtual void OnReset() = 0;

		std::string m_SourcePath;

	private:
		Project(const Project &);
		Project &operator=(const Project &);

		void CloseWorkspace();
		void NewWorkspace();
		void RefreshWorkspace();
		static std::string DirNameOf(const std::string &path);
		static std::string ResolvePath(const std::string &path);
		static void AsciiPathToLower(std::string &value);

		class Parts;

		Format m_Format;
		std::vector<Part *> m_Parts;
		Folder *m_Workspace;
		std::vector<Folder *> m_Retired;
		Identity m_Identity;
	};

} // namespace Spumoni

#endif
