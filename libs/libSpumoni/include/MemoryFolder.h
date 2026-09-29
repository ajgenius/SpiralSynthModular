// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::MemoryFolder — a working tree held entirely in memory.
//
// Nothing outside the process can open it: there is no path, no mount, no
// file record. Entries are byte strings in a map; streaming goes through
// memory streams. The one way out is PathFor, for code that can only take
// a path (a plugin reading a sample): that entry alone is written into a
// private disk scratch the folder owns and removes with itself, and is
// rewritten if the entry changes. Bytes() says how much the tree holds, so
// a caller can decide to work on disk instead when a package is too large.
#ifndef SPUMONI_MEMORY_FOLDER_H
#define SPUMONI_MEMORY_FOLDER_H

#include "Folder.h"

#include <map>
#include <string>

namespace Spumoni
{

	class MemoryFolder : public Folder
	{
	public:
		MemoryFolder();
		virtual ~MemoryFolder();

		// * The bytes held by every entry.
		unsigned long long Bytes() const { return m_Bytes; }

		virtual const char *Kind() const { return "memory"; }
		virtual bool Create(const std::string &prefix, std::string &error);
		virtual bool IsOpen() const { return m_Open; }
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
		struct Entry
		{
			bool Directory;
			std::string Data;
			std::string Materialised;	// relative path in the scratch, when PathFor made one
			Entry() : Directory(false) {}
		};
		struct Pending
		{
			std::string Relative;
			char *Buffer;
			size_t Size;
		};

		static std::string Normalise(const std::string &relative);
		static bool ValidRelative(const std::string &relative);
		bool MakeParents(const std::string &relative, std::string &error);
		void Forget(const std::string &relative);

		bool m_Open;
		std::string m_Prefix;
		std::map<std::string, Entry> m_Entries;
		std::map<FILE *, Pending> m_Pending;
		DiskFolder *m_Scratch;
		unsigned long long m_Bytes;
	};

} // namespace Spumoni

#endif
