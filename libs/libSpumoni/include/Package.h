// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Package — the layered package: what the layers are, how they are
// arranged in a container, and the version of that arrangement.
//
// Three clocks turn independently in a package file:
//   the container   (ZIP, tar, a directory: identified by what it is),
//   the package     ("Spumoni Package Ver N": how the tree is arranged),
//   the payloads    (the application's own headers on the manifest and on
//                    every file in a branch: what the entries mean).
// This class owns the middle one. Ver 1 is the layout in use today:
//
//   <container>
//   ├── <manifest>                the package's identity and the app's part
//   └── branches/<uuid>/          one folder per save point
//       ├── <payload files>       the application's, written through Payload
//       └── <anything else>       carried forward from the branch it came from
//
// Ver 2 adds package-wide content-addressed assets; Ver 3 checkpoints and a
// replayable journal. The application's payload versions never move for any
// of that, and a package bump never moves them. A reader refuses a newer
// stamp at its own level and says which level.
//
// Package knows nothing of the application: Layout names its files,
// Application writes and checks its part of the manifest, Payload writes a
// branch. An application (SpiralSynthModular's Archive) is those three
// things and calls Open / Write / WriteManifest / BranchesIn.
#ifndef SPUMONI_PACKAGE_H
#define SPUMONI_PACKAGE_H

#include "Container.h"
#include "Identity.h"

#include <string>
#include <vector>

#include "Folder.h"

namespace Slick { class JSONValue; }

namespace Spumoni
{

	class Package
	{
	public:
		// ---- The package format's own clock. ----------------------------
		static const long FormatVersion = 1;

		// * "Spumoni Package Ver 1": the manifest's "package" member.
		static std::string FormatName();
		static std::string Stamp();
		static std::string Stamp(long version);

		enum Status
		{
			Current,	// exactly the version this build writes
			Older,		// an earlier version this build still reads
			Newer,		// written by a later build: reject
			Foreign		// not a Spumoni stamp at all
		};

		// * Parse a stamp; `found` receives N when it is one of ours.
		static Status Check(const std::string &stamp, long &found);

		// * User-facing reason for a non-Current status.
		static std::string Reason(Status status, long found);

		// ---- What the application supplies. ------------------------------

		// * Names. The manifest file at the root; the files the application
		//   owns inside a branch (everything else there is an extra, carried
		//   forward untouched); the prefix of the working folders made for it.
		struct Layout
		{
			std::string ManifestName;
			std::vector<std::string> Payload;
			std::string WorkPrefix;
			// The kind of working folder Package makes for itself (a source
			// package extracted for a look, a manifest rewrite). NULL: disk.
			Folder *(*MakeFolder)();
			Layout() : MakeFolder(NULL) {}
		};

		// * The application's part of the manifest: its own stamp and
		//   metadata beside the package's members. Describe adds members to
		//   the root being written; Accept checks a root being read and takes
		//   what it wants from it (the author, say) into the identity.
		class Application
		{
		public:
			virtual ~Application() {}
			virtual void Describe(Slick::JSONValue &root, const Identity &identity) const = 0;
			virtual bool Accept(const Slick::JSONValue &root, Identity &identity, std::string &error) const = 0;
		};

		// * What goes into the active branch on a write. `branchRoot` is
		//   "branches/<uuid>/"; the payload adds its files under it.
		class Payload
		{
		public:
			virtual ~Payload() {}
			virtual bool Write(Container::Writer &writer, const std::string &branchRoot, std::string &error) = 0;
		};

		// ---- The package. -------------------------------------------------
		Package(const Container &container, const Layout &layout, const Application &application);

		const Layout &Files() const { return m_Layout; }

		// * Read. Unpack the whole package at `path` into the given, not yet
		//   created, working folder (the caller picks its kind), read the
		//   manifest, choose the branch (an empty id selects the active one).
		//   `branchRoot` receives "branches/<uuid>/". On failure the folder is
		//   removed with everything in it.
		bool Open(const std::string &path, const std::string &branchId,
			Folder &folder, Identity &identity, std::string &branchRoot, std::string &error) const;

		// * Write the package at `path` from an identity as given: the payload
		//   as the active branch, the other branch folders and the source
		//   branch's extras carried from the request's package or workspace,
		//   the manifest, the file replaced atomically.
		bool Write(const std::string &path, const Identity &identity, const SaveRequest &request,
			Payload &payload, std::string &error) const;

		// * The branch folders present in what a write would preserve from.
		std::vector<std::string> BranchesIn(const SaveRequest &request, std::string &error) const;

		// * Rewrite only the manifest of an existing package.
		bool WriteManifest(const std::string &path, const Identity &identity, std::string &error) const;

		// * Manifest text for an identity, as Write and WriteManifest emit it.
		std::string ManifestJSON(const Identity &identity) const;

		// * Read the manifest of an unpacked package, or manifest text: both
		//   stamps checked, the identity filled, the active branch root returned.
		bool ReadManifest(const Folder &folder, Identity &identity, std::string &branchRoot, std::string &error) const;
		bool ReadManifestText(const std::string &text, Identity &identity, std::string &branchRoot, std::string &error) const;

		// * The branch folders in an unpacked package.
		static std::vector<std::string> ListBranches(const Folder &source);

		// * A working folder of the layout's kind; the caller owns it.
		Folder *NewFolder() const;

		static std::string BranchRoot(const std::string &id) { return "branches/" + id + "/"; }

	private:
		Package(const Package &);
		Package &operator=(const Package &);

		// The source a write preserves from, or NULL when there is none.
		bool OpenPreserveSource(const SaveRequest &request, Folder *&source, std::string &error) const;
		bool CopyPreservedBranches(Container::Writer &writer, const Folder &source, const std::string &rewriteID, std::string &error) const;
		bool CopyActiveExtras(Container::Writer &writer, const Folder &source, const std::string &sourceID, const std::string &activeID, std::string &error) const;
		static bool ReadActiveBranch(const Slick::JSONValue &root, const std::string &activeBranch, std::string &activePath, std::string &error);
		static std::string SaveTimeUTC();

		const Container &m_Container;
		Layout m_Layout;
		const Application &m_Application;
	};

} // namespace Spumoni

#endif
