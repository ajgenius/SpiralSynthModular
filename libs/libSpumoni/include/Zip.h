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

#include "Container.h"

#include <cstdio>
#include <set>
#include <stdint.h>
#include <string>
#include <vector>

namespace Spumoni
{

	class Zip : public Container
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

		class Writer : public Container::Writer
		{
		public:
			Writer();
			virtual ~Writer();

			virtual bool Open(const std::string &path, std::string &error);
			virtual bool AddMemory(const std::string &name, const std::string &data, std::string &error);
			virtual bool AddFile(const std::string &name, const std::string &path, std::string &error);
			virtual bool AddDirectory(const std::string &name, std::string &error);
			virtual bool Finish(const std::string &path, std::string &error);

		private:
			bool Add(const std::string &name, const unsigned char *memory, size_t memorySize,
				FILE *input, std::string &error);

			FILE *m_File;
			std::string m_Path;
			std::vector<Entry> m_Entries;
			std::set<std::string> m_Names;
		};

		// * The Container interface.
		virtual const char *Kind() const;
		virtual Container::Writer *NewWriter() const;
		virtual bool Extract(const std::string &path, Folder &folder, std::string &error) const;

		// * The pieces of Extract, for callers that want to stop between them.
		static bool ReadCentralDirectory(FILE *file, std::vector<Entry> &entries, std::string &error);
		static bool ExtractEntry(FILE *archive, const Entry &entry,
			Folder &folder, std::string &error);
	};

} // namespace Spumoni

#endif
