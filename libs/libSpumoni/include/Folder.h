// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Folder — the working tree a package is unpacked into, edited in
// and packed back from.
//
// Everything between the container and the files goes through here, so the
// place we work before saving is one decision: a private temporary tree on
// disk today (Folder), a memory-backed one or a fixed directory tomorrow by
// subclassing Create/Remove. A Folder owns its tree: it is removed when the
// Folder goes out of scope unless Release() hands the path to someone else.
#ifndef SPUMONI_FOLDER_H
#define SPUMONI_FOLDER_H

#include <string>
#include <vector>

namespace Spumoni
{

	class Folder
	{
	public:
		Folder();
		virtual ~Folder();

		// * Make a fresh private tree. The prefix names it (a temporary
		//   directory "<tmp>/<prefix>XXXXXX"); an empty prefix uses "spumoni-".
		virtual bool Create(const std::string &prefix, std::string &error);

		// * The tree's path while owned or released; empty when there is none.
		const std::string &Path() const { return m_Path; }
		bool Owned() const { return m_Owned; }

		// * Hand the tree to the caller: Path() stays, the destructor leaves
		//   it alone. RemoveReleased is the matching prefix-guarded removal for
		//   a path that came out of Release, so a stray path is never deleted.
		std::string Release();
		static void RemoveReleased(std::string &path, const std::string &prefix);

		// * Remove the tree now (also what the destructor does when owned).
		virtual void Remove();

		// * Tree helpers used by containers and packages alike.
		static std::string TemporaryRoot();
		static std::string ErrnoText(const std::string &what);
		static bool IsDirectory(const std::string &path);
		static bool IsFile(const std::string &path);
		static bool List(const std::string &path, std::vector<std::string> &names);
		static bool MakeDirectories(const std::string &path, std::string &error);
		static bool RemoveTree(const std::string &path);
		static bool WriteFile(const std::string &path, const std::string &data, std::string &error);

	private:
		Folder(const Folder &);
		Folder &operator=(const Folder &);

		std::string m_Path;
		bool m_Owned;
	};

} // namespace Spumoni

#endif
