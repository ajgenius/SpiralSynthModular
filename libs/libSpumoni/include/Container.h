// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPUMONI_CONTAINER_H
#define SPUMONI_CONTAINER_H

#include <string>

namespace Spumoni
{
	class Folder;

	class Container
	{
	public:
		virtual ~Container() {}

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

		virtual Writer *NewWriter() const = 0;

		virtual bool Extract(const std::string &path, Folder &folder, std::string &error) const = 0;

		static const Container *Sniff(const std::string &path);
		static const Container &ForPath(const std::string &path);

		static bool AddTree(Writer &writer, const std::string &diskPath,
			const std::string &archivePath, bool root, std::string &error);
		static bool AddTreeExact(Writer &writer, const std::string &diskPath,
			const std::string &archivePath, std::string &error);

		static std::string ErrnoText(const std::string &what);
		static std::string BaseName(const std::string &path);
		static std::string SafeName(const std::string &value, const std::string &fallback);
		static bool SafeArchivePath(const std::string &name);
	};

} // namespace Spumoni

#endif
