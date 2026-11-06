// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPUMONI_FOLDER_H
#define SPUMONI_FOLDER_H

#include "Container.h"
#include "Store.h"
#include <cstdio>
#include <string>
#include <vector>

namespace Spumoni
{

	class Folder
	{
	public:
		virtual ~Folder() {}

		virtual const char *Kind() const = 0;

		virtual bool Create(const std::string &prefix, std::string &error) = 0;
		virtual bool IsOpen() const = 0;
		virtual void Remove() = 0;

		virtual bool IsDirectory(const std::string &relative) const = 0;
		virtual bool IsFile(const std::string &relative) const = 0;
		virtual bool List(const std::string &relative, std::vector<std::string> &names) const = 0;
		virtual bool MakeDirectory(const std::string &relative, std::string &error) = 0;
		virtual bool Read(const std::string &relative, std::string &data, std::string &error) const = 0;
		virtual bool Write(const std::string &relative, const std::string &data, std::string &error) = 0;
		virtual bool RemoveEntry(const std::string &relative) = 0;

		virtual FILE *OpenWrite(const std::string &relative, std::string &error) = 0;
		virtual bool CloseWrite(FILE *file, std::string &error) = 0;
		virtual FILE *OpenRead(const std::string &relative, std::string &error) const = 0;

		virtual bool PathFor(const std::string &relative, std::string &path, std::string &error) = 0;

		virtual bool AddTo(Container::Writer &writer, const std::string &relative,
			const std::string &archiveName, std::string &error) const = 0;

		bool ImportTree(const std::string &diskPath, const std::string &relative, std::string &error);
		bool ExportTree(Container::Writer &writer, const std::string &relative,
			const std::string &archivePath, std::string &error) const;

		virtual Store *GetStore() { return NULL; }

	protected:
		Folder() {}

	private:
		Folder(const Folder &);
		Folder &operator=(const Folder &);
	};

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

	class DiskFolder : public Folder
	{
	public:
		DiskFolder();
		virtual ~DiskFolder();

		static DiskFolder *Adopt(const std::string &path);

		const std::string &Root() const { return m_Path; }
		bool Owned() const { return m_Owned; }

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

		virtual Store *GetStore();

	private:
		std::string m_Path;
		bool m_Owned;
		Store *m_Store;
		std::string m_StoreRoot;
	};

} // namespace Spumoni

#endif
