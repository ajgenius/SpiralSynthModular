// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPUMONI_TAR_H
#define SPUMONI_TAR_H

#include "Container.h"

#include <set>
#include <string>

namespace Spumoni
{

	class Tar : public Container
	{
	public:
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
			bool Header(const std::string &name, unsigned long long size, char type, std::string &error);
			bool Body(const unsigned char *memory, size_t memorySize, FILE *input,
				unsigned long long size, std::string &error);
			bool Put(const void *data, size_t size, std::string &error);

			void *m_File;
			std::string m_Path;
			std::set<std::string> m_Names;
		};

		virtual const char *Kind() const;
		virtual Container::Writer *NewWriter() const;
		virtual bool Extract(const std::string &path, Folder &folder, std::string &error) const;

		static bool Compressed(const std::string &path);
	};

} // namespace Spumoni

#endif
