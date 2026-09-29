// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Directory — a bare folder as the container.
//
// The package tree written straight to disk: no archive, every entry a
// file. Useful for a package kept under version control, for inspecting
// one, and as the simplest proof that nothing above Container depends on
// ZIP. Writing builds a sibling folder and swaps it into place on Finish;
// extracting copies the tree into the working folder.
#ifndef SPUMONI_DIRECTORY_H
#define SPUMONI_DIRECTORY_H

#include "Container.h"

#include <set>
#include <string>

namespace Spumoni
{

	class Directory : public Container
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
			bool Place(const std::string &name, std::string &target, std::string &error);

			std::string m_Path;
			std::set<std::string> m_Names;
		};

		virtual const char *Kind() const;
		virtual Container::Writer *NewWriter() const;
		virtual bool Extract(const std::string &path, const std::string &folder, std::string &error) const;

		// * Copy one file or a whole tree; used by Extract and by anyone
		//   moving a package between folders.
		static bool CopyFile(const std::string &from, const std::string &to, std::string &error);
		static bool CopyTree(const std::string &from, const std::string &to, std::string &error);
	};

} // namespace Spumoni

#endif
