// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Package — the layered package format, and its own version.
//
// Three clocks turn independently in a package file:
//   the container   (ZIP, tar, a directory: identified by what it is),
//   the package     ("Spumoni Package Ver N": how the tree is arranged),
//   the payloads    (the application's own headers on the manifest and on
//                    every file in a branch: what the entries mean).
// This header owns the middle one. Ver 1 is the layout in use today: a
// manifest at the root, save points as branches/<id>/ folders, each with
// its assets beside it, anything else in a branch carried along untouched.
// Ver 2 adds package-wide content-addressed assets; Ver 3 checkpoints and a
// replayable journal. The application's payload versions never move for
// any of that, and a package bump never moves them.
//
// A reader refuses a newer stamp at its own level and says which level.
#ifndef SPUMONI_PACKAGE_H
#define SPUMONI_PACKAGE_H

#include <string>

namespace Spumoni
{

	class Package
	{
	public:
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
	};

} // namespace Spumoni

#endif
