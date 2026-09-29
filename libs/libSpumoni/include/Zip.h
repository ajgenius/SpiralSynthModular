// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Zip — a ZIP32 container.
//
// Spumoni is the layered package: a manifest, save points as branch folders,
// assets beside them, all boxed in a container. This is the box. It knows
// entries, offsets and CRCs and nothing about what the entries mean, so a
// tarball or a bare directory can stand in its place later.
//
// Writer builds a new archive entry by entry and replaces the target
// atomically on Finish; the static readers extract by central directory.
// Limits: ZIP32, single disk, stored or deflated entries, no encryption.
#ifndef SPUMONI_ZIP_H
#define SPUMONI_ZIP_H

#include <cstdio>
#include <set>
#include <stdint.h>
#include <string>
#include <vector>

namespace Spumoni
{

	class Zip
	{
	public:
		static const uint64_t kMaxEntrySize = uint64_t(2) * 1024 * 1024 * 1024;
		static const uint64_t kMaxArchiveSize = uint64_t(4) * 1024 * 1024 * 1024 - 1;

		struct Entry
		{
			std::string Name;
			uint32_t CRC, Compressed, Size, Offset;
			uint16_t Method;
		};

		// * Little-endian field helpers.
		static bool Write16(FILE *file, uint16_t value);
		static bool Write32(FILE *file, uint32_t value);
		static uint16_t Read16(const unsigned char *p);
		static uint32_t Read32(const unsigned char *p);
		static bool Seek(FILE *file, uint64_t offset);
		static bool Tell(FILE *file, uint32_t &offset);

		// * Path helpers. SafeArchivePath admits only relative, normalised,
		//   forward-slash names; SafeName folds anything else to [-_.A-Za-z0-9].
		static std::string ErrnoText(const std::string &what);
		static std::string BaseName(const std::string &path);
		static std::string SafeName(const std::string &value, const std::string &fallback);
		static bool SafeArchivePath(const std::string &name);

		class Writer
		{
		public:
			Writer();
			~Writer();

			bool Open(const std::string &path, std::string &error);
			bool AddMemory(const std::string &name, const std::string &data, std::string &error);
			bool AddFile(const std::string &name, const std::string &path, std::string &error);
			bool AddDirectory(const std::string &name, std::string &error);
			bool Finish(const std::string &path, std::string &error);

		private:
			bool Add(const std::string &name, const unsigned char *memory, size_t memorySize,
				FILE *input, std::string &error);

			FILE *m_File;
			std::string m_Path;
			std::vector<Entry> m_Entries;
			std::set<std::string> m_Names;
		};

		// * Whole trees into a Writer: AddTree sanitises child names (assets
		//   from anywhere on disk), AddTreeExact keeps them (our own layout).
		static bool AddTree(Writer &zip, const std::string &diskPath,
			const std::string &archivePath, bool root, std::string &error);
		static bool AddTreeExact(Writer &zip, const std::string &diskPath,
			const std::string &archivePath, std::string &error);

		// * Reading. ExtractTo unpacks every entry into an existing folder (a
		//   Spumoni::Folder, usually); the pieces are exposed for callers that
		//   want to stop between them. On failure the folder is left as it is.
		static bool ExtractTo(const char *path, const std::string &folder, std::string &error);
		static bool ReadCentralDirectory(FILE *file, std::vector<Entry> &entries, std::string &error);
		static bool ExtractEntry(FILE *archive, const Entry &entry,
			const std::string &workspace, std::string &error);
	};

} // namespace Spumoni

#endif
