// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPUMONI_PROJECT_H
#define SPUMONI_PROJECT_H

#include "Folder.h"
#include "Identity.h"
#include "Package.h"

#include <string>
#include <vector>

namespace Spumoni
{

	class Part
	{
	public:
		virtual ~Part() {}

		virtual std::string Name() const = 0;
		virtual std::string LegacyName() const { return std::string(); }
		virtual bool Required() const { return true; }

		virtual bool Load(Folder &folder, const std::string &branchRoot,
			const std::string &packagePath, std::string &error) = 0;
		virtual void Commit() = 0;
		virtual void Discard() = 0;
		virtual bool Store(Container::Writer &writer, const std::string &branchRoot,
			class Store *shared, std::string &error) = 0;
	};

	struct Format
	{
		std::string ManifestName;
		std::string MetadataName;
		std::string MetadataKey;
		std::string LegacyManifestName;
		const Package::Application *Application;
		std::vector<std::string> Extensions;
		std::string WorkPrefix;
		unsigned long long MemoryLimit;
		Format() : Application(NULL), MemoryLimit(256ULL * 1024 * 1024) {}
	};

	class Project
	{
	public:
		virtual ~Project();

		void Reset(const std::string &path = std::string());

		bool OpenPackage(const std::string &branchId, std::string &error);
		bool OpenSavePoint(const std::string &branchId, std::string &error);
		bool SwitchBranch(const std::string &branchId, std::string &error);

		bool SaveAs(const std::string &path, std::string &error);
		bool CreateSavePoint(const std::string &branchName, bool replaceIfExists, std::string &error, bool independent = false);
		bool UpdateManifest(std::string &error);

		bool WritePackage(const std::string &path, SaveRequest request, std::string &error);

		void AdoptIdentity(const Identity &identity) { m_Identity = identity; }
		const Identity &GetIdentity() const { return m_Identity; }
		Identity &GetIdentity() { return m_Identity; }
		const std::string &SourcePath() const { return m_SourcePath; }

		const Folder *Workspace() const { return m_Workspace; }
		Folder *Workspace() { return m_Workspace; }
		Folder *ReleaseWorkspace();
		static void DestroyWorkspace(Folder *&workspace);

		bool HasPackagePath() const { return LooksLikePackage(m_SourcePath); }

		bool LooksLikePackage(const std::string &path) const;
		bool IsPackage(const std::string &path) const;
		std::string BranchNameFromPath(const std::string &path) const;
		static bool SameFile(const std::string &a, const std::string &b);
		static std::string SystemUsername();
		static std::string SuggestedSavePointName(const Identity &identity) { return identity.SuggestedName(); }

		const Format &GetFormat() const { return m_Format; }
		std::string PackageWord() const;
		Package::Layout Layout() const;
		Folder *NewWorkspaceFor(const std::string &path) const;

	protected:
		explicit Project(const Format &format);

		void AddPart(Part &part);

		virtual bool HasContent() const = 0;
		virtual void OnReset() = 0;

		std::string m_SourcePath;

	private:
		Project(const Project &);
		Project &operator=(const Project &);

		void CloseWorkspace();
		void NewWorkspace();
		void RefreshWorkspace();
		static std::string DirNameOf(const std::string &path);
		static std::string ResolvePath(const std::string &path);
		static void AsciiPathToLower(std::string &value);

		class Parts;

		Format m_Format;
		std::vector<Part *> m_Parts;
		Folder *m_Workspace;
		std::vector<Folder *> m_Retired;
		Identity m_Identity;
	};

} // namespace Spumoni

#endif
