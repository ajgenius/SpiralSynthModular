// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Project — the save surface of a layered package.
// Grown out of Spiral::File::Project (ChatGPT GPT-5.6 / GROK originals):
// identity resolution, save points, workspace ownership, path judgements.

#include "Project.h"
#include "Folder.h"
#include "MemoryFolder.h"

#include <memory>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <pwd.h>
#include <unistd.h>

using namespace std;

namespace Spumoni
{

	// The parts as one Package::Payload: each stores itself into the branch.
	class Project::Parts : public Package::Payload
	{
	public:
		Parts(const std::vector<Part *> &parts, Store *shared) : m_Parts(parts), m_Shared(shared) {}
		virtual bool Write(Container::Writer &writer, const std::string &branchRoot, std::string &error)
		{
			for (size_t i = 0; i < m_Parts.size(); ++i)
				if (!m_Parts[i]->Store(writer, branchRoot, m_Shared, error))
					return false;
			return true;
		}
	private:
		const std::vector<Part *> &m_Parts;
		Store *m_Shared;
	};

	Project::Project(const Format &format)
		: m_SourcePath()
		, m_Format(format)
		, m_Workspace(NULL)
		, m_Identity()
	{
		NewWorkspace();
	}

	Project::~Project()
	{
		CloseWorkspace();
		for (size_t i = 0; i < m_Retired.size(); ++i)
			delete m_Retired[i];
	}

	void Project::AddPart(Part &part)
	{
		m_Parts.push_back(&part);
	}

	// How the format's package is called in messages: ".ssmp" or "package".
	std::string Project::PackageWord() const
	{
		return m_Format.Extensions.empty() ? std::string("package") : m_Format.Extensions[0];
	}

	Package::Layout Project::Layout() const
	{
		Package::Layout layout;
		layout.ManifestName = m_Format.ManifestName;
		layout.MetadataName = m_Format.MetadataName;
		layout.MetadataKey = m_Format.MetadataKey;
		layout.LegacyManifestName = m_Format.LegacyManifestName;
		for (size_t i = 0; i < m_Parts.size(); ++i)
			{
			layout.Payload.push_back(m_Parts[i]->Name());
			if (!m_Parts[i]->LegacyName().empty())
				layout.Payload.push_back(m_Parts[i]->LegacyName());
		}
		// Ver 2: "assets" is package-root content-addressed storage (shared across
		// branches), not a per-branch payload member. Handled via Folder::GetStore()
		// and exported by Package::Write at the package root level.
		layout.WorkPrefix = m_Format.WorkPrefix;
		return layout;
	}

	void Project::Reset(const std::string &path)
	{
		CloseWorkspace();
		OnReset();
		m_SourcePath = path;
		NewWorkspace();
	}

	/* A new project's working folder: in memory, laid out as a package with
	   one branch, the identity begun so the first save keeps it. */
	void Project::NewWorkspace()
	{
		m_Identity.Begin();
		std::string error;
		std::auto_ptr<Folder> folder(new MemoryFolder);
		if (!folder->Create(m_Format.WorkPrefix, error)
		    || !folder->MakeDirectory(Package::BranchRoot(m_Identity.ActiveBranchID), error))
			return;
		m_Workspace = folder.release();
	}

	/* After a write the file is the truth; unpack it again so the workspace
	   mirrors it (the branch just written included) and the next save point
	   preserves from something complete. The parts are not reloaded. Paths
	   materialised from the old workspace may still be in use, so it is
	   retired rather than destroyed. */
	void Project::RefreshWorkspace()
	{
		if (m_Workspace)
		{
			m_Retired.push_back(m_Workspace);
			m_Workspace = NULL;
		}
		Package package(Container::ForPath(m_SourcePath), Layout(), *m_Format.Application);
		std::auto_ptr<Folder> workspace(NewWorkspaceFor(m_SourcePath));
		Identity ignored;
		std::string branchRoot, error;
		if (package.Open(m_SourcePath, m_Identity.ActiveBranchID, *workspace, ignored, branchRoot, error))
			m_Workspace = workspace.release();
	}

	Folder *Project::ReleaseWorkspace()
	{
		Folder *released = m_Workspace;
		m_Workspace = NULL;
		return released;
	}

	void Project::DestroyWorkspace(Folder *&workspace)
	{
		delete workspace;
		workspace = NULL;
	}

	void Project::CloseWorkspace()
	{
		delete m_Workspace;
		m_Workspace = NULL;
		m_Identity.Clear();
	}

	// Memory, unless the package is larger than the format allows there.
	Folder *Project::NewWorkspaceFor(const std::string &path) const
	{
		unsigned long long bytes = Path::IsDirectory(path)
			? Path::TreeBytes(path) : Path::FileBytes(path);
		if (bytes > m_Format.MemoryLimit) return new DiskFolder;
		return new MemoryFolder;
	}

	/* Unpack into a fresh workspace, load every part into its staging, and
	   only then commit: the prior content, workspace and identity are
	   replaced together or not at all. */
	bool Project::OpenPackage(const std::string &branchId, std::string &error)
	{
		error.clear();
		if (m_SourcePath.empty())
		{
			error = "No project path supplied";
			return false;
		}
		Package package(Container::ForPath(m_SourcePath), Layout(), *m_Format.Application);
		std::auto_ptr<Folder> workspace(NewWorkspaceFor(m_SourcePath));
		Identity identity;
		std::string branchRoot;
		if (!package.Open(m_SourcePath, branchId, *workspace, identity, branchRoot, error))
			return false;

		for (size_t i = 0; i < m_Parts.size(); ++i)
		{
			Part &part = *m_Parts[i];
			if (!workspace->IsFile(branchRoot + part.Name())
				&& !workspace->IsDirectory(branchRoot + part.Name()))
			{
				if (part.Required())
				{
					error = "Active branch has no " + part.Name();
					break;
				}
				continue;
			}
			if (!part.Load(*workspace, branchRoot, m_SourcePath, error))
				break;
		}
		if (!error.empty())
		{
			for (size_t i = 0; i < m_Parts.size(); ++i)
				m_Parts[i]->Discard();
			return false;
		}
		CloseWorkspace();
		for (size_t i = 0; i < m_Parts.size(); ++i)
			m_Parts[i]->Commit();
		m_Workspace = workspace.release();
		m_Identity = identity;
		return true;
	}

	bool Project::OpenSavePoint(const std::string &branchId, std::string &error)
	{
		error.clear();
		if (m_SourcePath.empty())
		{
			error = "No project path supplied";
			return false;
		}
		if (branchId.empty())
		{
			error = "OpenSavePoint requires a branch id";
			return false;
		}
		if (!LooksLikePackage(m_SourcePath))
		{
			error = "OpenSavePoint requires a saved " + PackageWord();
			return false;
		}
		return OpenPackage(branchId, error);
	}

	bool Project::SwitchBranch(const std::string &branchId, std::string &error)
	{
		return OpenSavePoint(branchId, error);
	}

	/* The package mechanics live here: which branch the write goes to,
	   what it is called, what the manifest lists. Package is then told the
	   identity to write and the sources to preserve from, nothing more. */
	bool Project::WritePackage(const std::string &path, SaveRequest request, std::string &error)
	{
		error.clear();
		if (!HasContent())
		{
			error = "WritePackage requires a loaded definition";
			return false;
		}
		if (path.empty())
		{
			error = "WritePackage requires a destination path";
			return false;
		}

		Package package(Container::ForPath(path), Layout(), *m_Format.Application);
		Identity next = m_Identity;
		if (next.PackageID.empty() && next.ActiveBranchID.empty())
			next.Begin();
		if (next.AuthorUsername.empty())
			next.AuthorUsername = SystemUsername();
		if (next.ActiveBranchName.empty())
			next.ActiveBranchName = BranchNameFromPath(path);
		next.EnsureActiveListed();

		// Branch folders the source holds that the manifest never listed
		// are kept and listed.
		std::vector<std::string> present = package.BranchesIn(request, error);
		if (!error.empty())
			return false;
		for (size_t i = 0; i < present.size(); ++i)
			next.AdoptUnlisted(present[i]);
		bool havePrevious = false;
		for (size_t i = 0; i < present.size(); ++i)
			if (present[i] == next.ActiveBranchID)
				havePrevious = true;

		request.SourceBranchID = next.ActiveBranchID;
		if (request.Mode == SaveNewBranch || request.Mode == SaveImportedBranch)
		{
			// The fork's extras come from the branch it forks from; where
			// it forked is the parent's checkpoint the live state came from.
			std::string name = request.NewBranchName;
			if (name.empty() && havePrevious)
				name = next.SuggestedName();
			if (havePrevious)
			{
				if (!next.Fork(name, error))
					return false;
			}
			else if (!name.empty())
				next.ActiveBranchName = name;
			next.EnsureActiveListed();
		}
		else if (!request.NewBranchName.empty())
		{
			// Replace: target the named branch and make it active.
			if (!next.ActivateNamed(request.NewBranchName, error))
				return false;
			next.EnsureActiveListed();
			request.SourceBranchID = next.ActiveBranchID;
		}

		if (request.Mode == SaveImportedBranch)
		{
			Branch *imported = next.Find(next.ActiveBranchID);
			imported->Kind = "imported";
			imported->ParentID.clear();
			imported->ForkSaveID.clear();
			request.SourceBranchID.clear();
		}

		// Ver 2: the parts may put bundled files into the workspace's store;
		// Package::Write then exports the store at the package root.
		Store *store = m_Workspace ? m_Workspace->GetStore() : NULL;
		request.SharedStore = store;
		Parts payload(m_Parts, store);
		if (!package.Write(path, next, request, payload, error))
			return false;

		m_SourcePath = path;
		m_Identity = next;
		error.clear();
		return true;
	}

	bool Project::SaveAs(const std::string &path, std::string &error)
	{
		error.clear();
		if (!HasContent())
		{
			error = "SaveAs requires a loaded definition";
			return false;
		}
		if (path.empty())
		{
			error = "SaveAs requires a destination path";
			return false;
		}

		// Same resolved package path → replace the first-save (basename) branch.
		if (LooksLikePackage(m_SourcePath) && LooksLikePackage(path)
			&& SameFile(m_SourcePath, path))
		{
			return CreateSavePoint(BranchNameFromPath(path), true, error);
		}

		// Different filename → a package of its own. A project saved before
		// gets a fresh identity (a copy is not the original); one never saved
		// keeps the identity it was born with, and whatever its working
		// folder holds beside the parts comes along. Either way no other
		// branches are carried into the new package.
		Identity previous = m_Identity;
		SaveRequest request;
		request.Mode = SaveReplace;
		if (LooksLikePackage(m_SourcePath))
		{
			m_Identity.Clear();
			m_Identity.ApplicationMetadata = previous.ApplicationMetadata;
			m_Identity.AuthorUsername = previous.AuthorUsername;
		}
		else if (m_Workspace)
			request.Workspace = m_Workspace;

		if (!WritePackage(path, request, error))
		{
			m_Identity = previous;
			return false;
		}
		RefreshWorkspace();
		error.clear();
		return true;
	}

	bool Project::CreateSavePoint(const std::string &branchName, bool replaceIfExists,
		std::string &error, bool independent)
	{
		error.clear();
		if (!HasContent())
		{
			error = "CreateSavePoint requires a loaded definition";
			return false;
		}
		if (branchName.empty())
		{
			error = "CreateSavePoint requires a branch name";
			return false;
		}
		if (!LooksLikePackage(m_SourcePath))
		{
			error = "CreateSavePoint requires a saved " + PackageWord() + " (use SaveAs first)";
			return false;
		}

		SaveRequest request;
		// Prefer the live workspace (may hold extras not yet in the package).
		if (m_Workspace)
			request.Workspace = m_Workspace;
		else
			request.ExistingPackage = m_SourcePath;
		request.NewBranchName = branchName;

		if (replaceIfExists)
			request.Mode = SaveReplace;
		else
			request.Mode = independent ? SaveImportedBranch : SaveNewBranch;

		// Replace activates the named branch inside WritePackage; new forks.
		// Identity updates only on WritePackage success.
		if (!WritePackage(m_SourcePath, request, error))
			return false;
		RefreshWorkspace();
		return true;
	}

	bool Project::UpdateManifest(std::string &error)
	{
		error.clear();
		if (!HasContent())
		{
			error = "UpdateManifest requires a loaded definition";
			return false;
		}
		if (!LooksLikePackage(m_SourcePath))
		{
			error = "UpdateManifest requires a saved " + PackageWord() + " file (no package to update)";
			return false;
		}
		Package package(Container::ForPath(m_SourcePath), Layout(), *m_Format.Application);
		return package.WriteManifest(m_SourcePath, m_Identity, error);
	}

	// ---- Path judgements -------------------------------------------------

	std::string Project::SystemUsername()
	{
		struct passwd *user = getpwuid(getuid());
		if (user && user->pw_name && *user->pw_name)
			return user->pw_name;
		const char *environment = getenv("USER");
		return environment && *environment ? environment : "unknown";
	}

	// One of the format's extensions, case-insensitively.
	bool Project::LooksLikePackage(const std::string &path) const
	{
		std::string lower = path;
		AsciiPathToLower(lower);
		for (size_t i = 0; i < m_Format.Extensions.size(); ++i)
		{
			const std::string &extension = m_Format.Extensions[i];
			if (lower.size() > extension.size()
			    && lower.compare(lower.size() - extension.size(), extension.size(), extension) == 0)
				return true;
		}
		return false;
	}

	// Whatever Spumoni recognises as a container; a bare folder only when it
	// holds our manifest, since any folder is a directory.
	bool Project::IsPackage(const std::string &path) const
	{
		const Container *container = Container::Sniff(path);
		if (!container) return false;
		if (std::strcmp(container->Kind(), "directory") != 0) return true;
		return Path::IsFile(path + "/" + m_Format.ManifestName)
			|| (!m_Format.LegacyManifestName.empty() && Path::IsFile(path + "/" + m_Format.LegacyManifestName));
	}

	// Basename without the format's extension (or "Untitled").
	std::string Project::BranchNameFromPath(const std::string &path) const
	{
		std::string trimmed = path;
		while (trimmed.size() > 1 && trimmed[trimmed.size() - 1] == '/')
			trimmed.erase(trimmed.size() - 1);
		std::string name = trimmed.substr(trimmed.find_last_of('/') + 1);
		std::string lower = name;
		AsciiPathToLower(lower);
		for (size_t i = 0; i < m_Format.Extensions.size(); ++i)
		{
			const std::string &extension = m_Format.Extensions[i];
			if (lower.size() > extension.size()
			    && lower.compare(lower.size() - extension.size(), extension.size(), extension) == 0)
			{
				name.erase(name.size() - extension.size());
				break;
			}
		}
		return name.empty() ? "Untitled" : name;
	}

	std::string Project::DirNameOf(const std::string &path)
	{
		size_t end = path.size();
		while (end && path[end - 1] == '/') --end;
		if (!end) return "/";
		size_t slash = path.rfind('/', end - 1);
		if (slash == std::string::npos) return ".";
		if (slash == 0) return "/";
		return path.substr(0, slash);
	}

	// realpath when possible; otherwise realpath(parent)+basename (unsaved target).
	std::string Project::ResolvePath(const std::string &path)
	{
		if (path.empty()) return std::string();
	#ifndef PATH_MAX
		char buf[4096];
	#else
		char buf[PATH_MAX];
	#endif
		if (realpath(path.c_str(), buf))
			return std::string(buf);
		std::string base = Container::BaseName(path);
		if (base.empty() || base == "." || base == "..")
			return path;
		if (realpath(DirNameOf(path).c_str(), buf))
		{
			std::string parent = buf;
			if (parent == "/") return std::string("/") + base;
			return parent + "/" + base;
		}
		return path;
	}

	void Project::AsciiPathToLower(std::string &value)
	{
		for (size_t i = 0; i < value.size(); ++i)
			if (value[i] >= 'A' && value[i] <= 'Z')
				value[i] = static_cast<char>(value[i] - 'A' + 'a');
	}

	// Same package file: resolved paths. Case-insensitive on macOS.
	bool Project::SameFile(const std::string &a, const std::string &b)
	{
		if (a.empty() || b.empty()) return false;
		std::string ra = ResolvePath(a);
		std::string rb = ResolvePath(b);
	#if defined(__APPLE__)
		AsciiPathToLower(ra);
		AsciiPathToLower(rb);
	#endif
		return ra == rb;
	}

} // namespace Spumoni
