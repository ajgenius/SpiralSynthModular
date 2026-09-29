// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Folder — the working tree a package is unpacked into, edited in
// and packed back from.
//
// Everything between the container and the files goes through here, by
// relative path ("branches/<id>/patch.json"), so the place we work before
// saving is one decision: a private temporary tree on disk (DiskFolder), or
// a tree held entirely in memory that nothing outside the process can open
// (MemoryFolder). A Folder owns its tree: it is gone when the Folder is.
//
// Code that can only take a path, a plugin reading a sample, say, asks
// PathFor; a disk folder answers with the file, a memory folder materialises
// that one entry into a private scratch it also owns. Nothing else ever
// sees a path.
//
// Path is the handful of plain filesystem helpers the disk implementations
// and the containers share; it is not the Folder interface.
#ifndef SPUMONI_FOLDER_H
#define SPUMONI_FOLDER_H

#include "Container.h"

#include <cstdio>
#include <string>
#include <vector>

namespace Spumoni
{

	class Folder
	{
	public:
		virtual ~Folder() {}

		// * "disk" or "memory".
		virtual const char *Kind() const = 0;

		// * Make a fresh private tree. The prefix names it where a name is
		//   visible (a disk folder "<tmp>/<prefix>XXXXXX"); an empty prefix
		//   means "spumoni-". Remove empties it; the destructor does too.
		virtual bool Create(const std::string &prefix, std::string &error) = 0;
		virtual bool IsOpen() const = 0;
		virtual void Remove() = 0;

		// * Entries by relative path. Directories are made with their parents;
		//   Write makes the parents of the file. List gives sorted child names.
		virtual bool IsDirectory(const std::string &relative) const = 0;
		virtual bool IsFile(const std::string &relative) const = 0;
		virtual bool List(const std::string &relative, std::vector<std::string> &names) const = 0;
		virtual bool MakeDirectory(const std::string &relative, std::string &error) = 0;
		virtual bool Read(const std::string &relative, std::string &data, std::string &error) const = 0;
		virtual bool Write(const std::string &relative, const std::string &data, std::string &error) = 0;
		virtual bool RemoveEntry(const std::string &relative) = 0;

		// * Streaming. OpenWrite gives a FILE* to fill; the entry exists only
		//   once CloseWrite succeeds. OpenRead's FILE* is fclose()d by the caller.
		virtual FILE *OpenWrite(const std::string &relative, std::string &error) = 0;
		virtual bool CloseWrite(FILE *file, std::string &error) = 0;
		virtual FILE *OpenRead(const std::string &relative, std::string &error) const = 0;

		// * A real path for one entry, for code that can only take a path.
		virtual bool PathFor(const std::string &relative, std::string &path, std::string &error) = 0;

		// * Copy one entry out into a container being written, the cheapest
		//   way this kind knows (a disk folder hands over the file, a memory
		//   folder the bytes).
		virtual bool AddTo(Container::Writer &writer, const std::string &relative,
			const std::string &archiveName, std::string &error) const = 0;

		// * Whole trees, over the interface: a disk tree into this folder
		//   (streamed, names kept), and this folder's tree into a container
		//   Writer (exact names, directories included).
		bool ImportTree(const std::string &diskPath, const std::string &relative, std::string &error);
		bool ExportTree(Container::Writer &writer, const std::string &relative,
			const std::string &archivePath, std::string &error) const;

	protected:
		Folder() {}

	private:
		Folder(const Folder &);
		Folder &operator=(const Folder &);
	};

	// Plain filesystem helpers for absolute paths.
	struct Path
	{
		static std::string TemporaryRoot();
		static std::string ErrnoText(const std::string &what);
		static bool IsDirectory(const std::string &path);
		static bool IsFile(const std::string &path);
		static bool List(const std::string &path, std::vector<std::string> &names);
		static bool MakeDirectories(const std::string &path, std::string &error);
		static bool RemoveTree(const std::string &path);
		static bool ReadFile(const std::string &path, std::string &data, std::string &error);
		static bool WriteFile(const std::string &path, const std::string &data, std::string &error);
		static std::string Join(const std::string &root, const std::string &relative);
		static unsigned long long FileBytes(const std::string &path);
		static unsigned long long TreeBytes(const std::string &path);
	};

	// A private temporary tree on disk.
	class DiskFolder : public Folder
	{
	public:
		DiskFolder();
		virtual ~DiskFolder();

		// * An existing directory as a Folder, not owned: for looking at a
		//   workspace or a package folder someone else keeps.
		static DiskFolder *Adopt(const std::string &path);

		// * The tree's root while owned or released; empty when there is none.
		const std::string &Root() const { return m_Path; }
		bool Owned() const { return m_Owned; }

		// * Hand the tree to the caller: Root() stays, the destructor leaves
		//   it alone. RemoveReleased is the matching prefix-guarded removal for
		//   a path that came out of Release, so a stray path is never deleted.
		std::string Release();
		static void RemoveReleased(std::string &path, const std::string &prefix);

		virtual const char *Kind() const { return "disk"; }
		virtual bool Create(const std::string &prefix, std::string &error);
		virtual bool IsOpen() const { return !m_Path.empty(); }
		virtual void Remove();
		virtual bool IsDirectory(const std::string &relative) const;
		virtual bool IsFile(const std::string &relative) const;
		virtual bool List(const std::string &relative, std::vector<std::string> &names) const;
		virtual bool MakeDirectory(const std::string &relative, std::string &error);
		virtual bool Read(const std::string &relative, std::string &data, std::string &error) const;
		virtual bool Write(const std::string &relative, const std::string &data, std::string &error);
		virtual bool RemoveEntry(const std::string &relative);
		virtual FILE *OpenWrite(const std::string &relative, std::string &error);
		virtual bool CloseWrite(FILE *file, std::string &error);
		virtual FILE *OpenRead(const std::string &relative, std::string &error) const;
		virtual bool PathFor(const std::string &relative, std::string &path, std::string &error);
		virtual bool AddTo(Container::Writer &writer, const std::string &relative,
			const std::string &archiveName, std::string &error) const;

	private:
		std::string m_Path;
		bool m_Owned;
	};

} // namespace Spumoni

#endif
