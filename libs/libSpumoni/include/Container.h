// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Container — the box a package travels in.
//
// A package is a tree of files; a container is one file holding that tree.
// ZIP is the container today because it is portable and random-access, but
// nothing above this class knows that: a tarball, a compressed tarball or a
// bare directory implement the same two operations. Writing goes through a
// Writer, entry by entry, finishing with an atomic replace of the target;
// reading unpacks the whole tree into a Folder. Entry names are relative
// forward-slash paths; SafeArchivePath is the rule every container enforces.
#ifndef SPUMONI_CONTAINER_H
#define SPUMONI_CONTAINER_H

#include <string>

namespace Spumoni
{

	class Container
	{
	public:
		virtual ~Container() {}

		// * A short lower-case name for the kind: "zip", "tar", "directory".
		virtual const char *Kind() const = 0;

		class Writer
		{
		public:
			virtual ~Writer() {}

			virtual bool Open(const std::string &path, std::string &error) = 0;
			virtual bool AddMemory(const std::string &name, const std::string &data, std::string &error) = 0;
			virtual bool AddFile(const std::string &name, const std::string &path, std::string &error) = 0;
			virtual bool AddDirectory(const std::string &name, std::string &error) = 0;
			virtual bool Finish(const std::string &path, std::string &error) = 0;
		};

		// * A new Writer for this kind; the caller owns it.
		virtual Writer *NewWriter() const = 0;

		// * Unpack every entry of the container at `path` into an existing
		//   folder. On failure the folder is left as it is.
		virtual bool Extract(const std::string &path, const std::string &folder, std::string &error) const = 0;

		// * Which kind a path is. Sniff looks at what is there (a directory, a
		//   ZIP or gzip signature, a ustar header) and returns NULL when none
		//   of ours; ForPath falls back to the extension (.zip, .tar, .tgz,
		//   .tar.gz, a trailing slash) and then to ZIP, so a new file can be
		//   written. Both return shared instances; never delete them.
		static const Container *Sniff(const std::string &path);
		static const Container &ForPath(const std::string &path);

		// * Whole trees into any Writer: AddTree sanitises child names (assets
		//   from anywhere on disk), AddTreeExact keeps them (our own layout).
		static bool AddTree(Writer &writer, const std::string &diskPath,
			const std::string &archivePath, bool root, std::string &error);
		static bool AddTreeExact(Writer &writer, const std::string &diskPath,
			const std::string &archivePath, std::string &error);

		// * Entry-name vocabulary. SafeArchivePath admits only relative,
		//   normalised, forward-slash names; SafeName folds anything else to
		//   [-_.A-Za-z0-9].
		static std::string ErrnoText(const std::string &what);
		static std::string BaseName(const std::string &path);
		static std::string SafeName(const std::string &value, const std::string &fallback);
		static bool SafeArchivePath(const std::string &name);
	};

} // namespace Spumoni

#endif
