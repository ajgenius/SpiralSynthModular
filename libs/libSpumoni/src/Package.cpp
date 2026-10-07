// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Package — the mechanics of the layered package: unpacking into a
// working folder, the manifest, carrying branch folders and extras forward,
// writing the active branch through the application's Payload.
// Grown out of Spiral::File::Archive, the SpiralSynthModular .ssmp package:
// Code generated/modified by ChatGPT (GPT-5.6).
// Save-time named branches generated/modified by GROK (Grok Build).

#include "Package.h"
#include "Folder.h"
#include "JSON.h"

#include <memory>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <time.h>

using namespace std;

namespace Spumoni
{
	typedef JSON J;

	namespace
	{
		const J *Member(const J &object, const char *name, J::Type type)
		{
			const J *value = object.GetType() == J::Object ? object.Get(name) : NULL;
			return value && value->GetType() == type ? value : NULL;
		}

		string StringOr(const J *object, const char *name, const string &fallback = string())
		{
			const J *value = object ? Member(*object, name, J::String) : NULL;
			return value ? value->Text() : fallback;
		}
	}

	// A read-only look at a Folder someone else owns, so OpenPreserveSource
	// can hand every source back the same way.
	class Package::View : public Folder
	{
	public:
		explicit View(const Folder &folder) : m_Folder(folder) {}
		virtual const char *Kind() const { return m_Folder.Kind(); }
		virtual bool Create(const string &, string &error) { error="A view is not created"; return false; }
		virtual bool IsOpen() const { return m_Folder.IsOpen(); }
		virtual void Remove() {}
		virtual bool IsDirectory(const string &relative) const { return m_Folder.IsDirectory(relative); }
		virtual bool IsFile(const string &relative) const { return m_Folder.IsFile(relative); }
		virtual bool List(const string &relative, vector<string> &names) const { return m_Folder.List(relative,names); }
		virtual bool MakeDirectory(const string &, string &error) { error="Read-only view"; return false; }
		virtual bool Read(const string &relative, string &data, string &error) const { return m_Folder.Read(relative,data,error); }
		virtual bool Write(const string &, const string &, string &error) { error="Read-only view"; return false; }
		virtual bool RemoveEntry(const string &) { return false; }
		virtual FILE *OpenWrite(const string &, string &error) { error="Read-only view"; return NULL; }
		virtual bool CloseWrite(FILE *, string &error) { error="Read-only view"; return false; }
		virtual FILE *OpenRead(const string &relative, string &error) const { return m_Folder.OpenRead(relative,error); }
		virtual bool PathFor(const string &, string &, string &error) { error="Read-only view"; return false; }
		virtual bool AddTo(Container::Writer &writer, const string &relative, const string &archiveName, string &error) const
		{ return m_Folder.AddTo(writer,relative,archiveName,error); }
	private:
		const Folder &m_Folder;
	};

	Package::Package(const Container &container, const Layout &layout, const Application &application)
		: m_Container(container)
		, m_Layout(layout)
		, m_Application(application)
	{
	}

	/* Branch folders present in a source package or workspace. */
	std::vector<std::string> Package::ListBranches(const Folder &source)
	{
		vector<string> ids;
		vector<string> names;
		if(!source.List("branches",names)) return ids;
		for(size_t i=0;i<names.size();++i)
		{
			const string &id=names[i];
			if(!IsUUID(id)) continue;
			if(source.IsDirectory("branches/"+id))
				ids.push_back(id);
		}
		return ids;
	}

	Folder *Package::NewFolder() const
	{
		return m_Layout.MakeFolder ? m_Layout.MakeFolder() : new DiskFolder;
	}

	bool Package::CopyPreservedBranches(Container::Writer &writer, const Folder &source,
		const string &rewriteID, string &error) const
	{
		vector<string> ids=ListBranches(source);
		for(size_t i=0;i<ids.size();++i)
		{
			if(ids[i]==rewriteID) continue;
			if(!source.ExportTree(writer,"branches/"+ids[i],"branches/"+ids[i],error))
				return false;
		}
		return true;
	}

	/* Anything in the source branch beside the application's payload files
	   carries over to the active branch as it is. */
	bool Package::CopyActiveExtras(Container::Writer &writer, const Folder &source,
		const string &sourceID, const string &activeID, string &error) const
	{
		if(sourceID.empty()) return true;
		string branchPath="branches/"+sourceID;
		if(!source.IsDirectory(branchPath)) return true;
		vector<string> names;
		if(!source.List(branchPath,names)){error="Cannot preserve branch metadata";return false;}
		for(size_t i=0;i<names.size();++i)
		{
			const string &name=names[i];
			bool payload=false;
			for(size_t p=0;p<m_Layout.Payload.size();++p)
				if(m_Layout.Payload[p]==name){payload=true;break;}
			if(payload) continue;
			if(!source.ExportTree(writer,branchPath+"/"+name,"branches/"+activeID+"/"+name,error))
				return false;
		}
		return true;
	}

	/* The source a write preserves from: the request's package, unpacked for
	   the look by whatever container it is, or its live workspace adopted in
	   place. The caller owns what comes back. */
	bool Package::OpenPreserveSource(const SaveRequest &request,
		Folder *&source, string &error) const
	{
		source=NULL;
		if(!request.ExistingPackage.empty())
		{
			if(const Container *kind=Container::Sniff(request.ExistingPackage))
			{
				std::auto_ptr<Folder> unpacked(NewFolder());
				if(!unpacked->Create(m_Layout.WorkPrefix,error)||
					!kind->Extract(request.ExistingPackage,*unpacked,error))
					return false;
				source=unpacked.release();
				return true;
			}
		}
		if(request.Workspace && request.Workspace->IsOpen())
		{
			source=new View(*request.Workspace);
			return true;
		}
		if(!request.ExistingWorkspace.empty())
		{
			if(Path::IsDirectory(request.ExistingWorkspace))
			{
				source=DiskFolder::Adopt(request.ExistingWorkspace);
				return true;
			}
		}
		if(!request.ExistingPackage.empty() || !request.ExistingWorkspace.empty() || request.Workspace)
		{
			error="Previous package/workspace is unavailable; cannot preserve its history"; return false;
		}
		return true;
	}

	string Package::SaveTimeUTC()
	{
		time_t value=time(NULL);struct tm result;
		if(!gmtime_r(&value,&result)) return "unknown";
		char text[32];
		if(!strftime(text,sizeof(text),"%Y-%m-%dT%H:%M:%SZ",&result))
			return "unknown";
		return text;
	}

	/* The manifest: the package's stamp and identity, then whatever the
	   application adds (its own stamp first among them). */
	string Package::ManifestJSON(const Identity &identity) const
	{
		JSONOwner root(J::MakeObject());
		root->SetOwned("package",J::MakeString(Stamp()));
		if (m_Layout.MetadataName.empty())
			m_Application.Describe(*root.get(),identity);
		else
		{
			J *metadata = J::MakeObject();
			metadata->SetOwned(m_Layout.MetadataKey, J::MakeString(m_Layout.MetadataName));
			root->SetOwned("metadata", metadata);
		}
		root->SetOwned("package_id",J::MakeString(identity.PackageID));
		root->SetOwned("saved_at",J::MakeString(SaveTimeUTC()));
		root->SetOwned("current_branch",J::MakeString(identity.ActiveBranchID));

		J *branchNames=J::MakeObject();
		J *branches=J::MakeArray();
		for(size_t i=0;i<identity.Branches.size();++i)
		{
			const Branch &item=identity.Branches[i];
			string name=item.Name;
			if(name.empty() && item.ID==identity.ActiveBranchID)
				name=identity.ActiveBranchName;
			branchNames->SetOwned(item.ID,J::MakeString(name));
			J *branch=J::MakeObject();
			branch->SetOwned("id",J::MakeString(item.ID));
			branch->SetOwned("kind",J::MakeString(item.Kind.empty()?"named":item.Kind));
			branch->SetOwned("path",J::MakeString(BranchRoot(item.ID)));
			if(item.ParentID.empty())
				branch->SetOwned("parent_id",J::MakeNull());
			else
				branch->SetOwned("parent_id",J::MakeString(item.ParentID));
			if(item.ForkSaveID.empty())
				branch->SetOwned("fork_save_id",J::MakeNull());
			else
				branch->SetOwned("fork_save_id",J::MakeString(item.ForkSaveID));
			branches->AppendOwned(branch);
		}
		J *names=J::MakeObject();
		names->SetOwned("project",J::MakeString(identity.ActiveBranchName));
		names->SetOwned("branches",branchNames);
		root->SetOwned("names",names);
		root->SetOwned("branches",branches);

		return root->Stringify(true);
	}

	string Package::MetadataJSON(const Identity &identity) const
	{
		JSONOwner root(J::MakeObject());
		m_Application.Describe(*root.get(), identity);
		return root->Stringify(true);
	}

	bool Package::ReadActiveBranch(const J &root, const string &activeBranch,
		string &activePath, string &error)
	{
		const J *branches=Member(root,"branches",J::Array);
		if(!branches)
		{error="Package manifest has no branches array";return false;}
		for(size_t i=0;i<branches->Size();++i)
		{
			const J *item=branches->At(i);
			if(item && StringOr(item,"id")==activeBranch)
			{
				activePath=StringOr(item,"path");
				break;
			}
		}
		if(activePath!=BranchRoot(activeBranch))
		{error="Active branch path does not match its UUID";return false;}
		return true;
	}

	bool Package::ReadManifest(const Folder &folder,
		Identity &identity, string &branchRoot, string &error) const
	{
		string name = m_Layout.ManifestName;
		if (!folder.IsFile(name) && !m_Layout.LegacyManifestName.empty())
			name = m_Layout.LegacyManifestName;

		string text;
		if (!folder.Read(name, text, error) || !ReadManifestText(text, identity, branchRoot, error))
			return false;

		if (name == m_Layout.LegacyManifestName || m_Layout.MetadataName.empty())
			return true;

		string metadata;
		if (!folder.Read(m_Layout.MetadataName, metadata, error))
			return false;
		JSONOwner root(ParseJSONText(metadata, &error));
		if (!root.get() || root->GetType() != J::Object)
		{
			error = "Invalid application metadata: " + m_Layout.MetadataName;
			return false;
		}
		if (!m_Application.Accept(*root.get(), identity, error))
			return false;
		identity.ApplicationMetadata = metadata;
		return true;
	}

	bool Package::ReadManifestText(const string &text,
		Identity &identity, string &branchRoot, string &error) const
	{
		string parseError;
		JSONOwner storage(ParseJSONText(text, &parseError));
		if(!storage.get()){error=parseError.empty()?"Cannot read package manifest":parseError;return false;}
		const J &root=*storage.get();
		if(root.GetType()!=J::Object){error="Not a package manifest";return false;}

		// The package level first: a newer arrangement is refused whatever
		// the payload says. A manifest without the stamp predates it and is
		// Ver 1, the only arrangement there was.
		if(const J *package=Member(root,"package",J::String))
		{
			long found=0;
			VersionStatus status=Check(package->Text(),found);
			if(status==Newer||status==Foreign)
			{error=m_Layout.ManifestName+": "+Reason(status,found);return false;}
		}

		// Then the application's part: its own stamp and members.
		const J *metadata = Member(root, "metadata", J::Object);
		if (!m_Layout.MetadataName.empty() && metadata)
		{
			if (StringOr(metadata, m_Layout.MetadataKey.c_str()) != m_Layout.MetadataName)
			{
				error = "Package does not register the expected application metadata";
				return false;
			}
		}
		else
		{
			if (!m_Application.Accept(root,identity,error)) return false;
			identity.ApplicationMetadata = text;
		}

		const string packageID=StringOr(&root,"package_id");
		const string activeBranch=StringOr(&root,"current_branch");
		if(!IsUUID(packageID))
		{error=m_Layout.ManifestName+" has an invalid package UUID";return false;}
		if(!IsUUID(activeBranch))
		{error=m_Layout.ManifestName+" has an invalid current branch UUID";return false;}
		string activePath;
		if(!ReadActiveBranch(root,activeBranch,activePath,error)) return false;

		const J *names=Member(root,"names",J::Object);
		const J *branchNames=names?Member(*names,"branches",J::Object):NULL;
		identity.ActiveBranchName=StringOr(branchNames,activeBranch.c_str());
		identity.Branches.clear();
		const J *branchList=Member(root,"branches",J::Array);
		for(size_t i=0;branchList&&i<branchList->Size();++i)
		{
			const J *item=branchList->At(i);
			if(!item||item->GetType()!=J::Object) continue;
			Branch branch;
			branch.ID=StringOr(item,"id");
			if(!IsUUID(branch.ID)) continue;
			branch.Kind=StringOr(item,"kind","named");
			branch.ParentID=StringOr(item,"parent_id");
			branch.ForkSaveID=StringOr(item,"fork_save_id");
			branch.Name=StringOr(branchNames,branch.ID.c_str());
			identity.Branches.push_back(branch);
		}
		identity.PackageID=packageID;identity.ActiveBranchID=activeBranch;
		branchRoot=activePath;return true;
	}

	/* Write the package: the payload as the active branch, every other
	   branch folder of the source carried over, the active branch's extras
	   (anything beside the payload files) carried from the source branch,
	   and the manifest. The identity is written as given. */
	bool Package::Write(const string &path, const Identity &identity,
		const SaveRequest &request, Payload &payload, string &error) const
	{
		error.clear();
		if(path.empty()){error="No package filename";return false;}
		if(!IsUUID(identity.PackageID)||!IsUUID(identity.ActiveBranchID))
		{error="Invalid package or active branch UUID";return false;}
		Folder *sourceFolder=NULL;
		if(!OpenPreserveSource(request,sourceFolder,error))
			return false;
		std::auto_ptr<Folder> source(sourceFolder);
		bool needPreserve=false;
		for(size_t i=0;i<identity.Branches.size();++i)
			if(identity.Branches[i].ID!=identity.ActiveBranchID){needPreserve=true;break;}
		if(needPreserve && !source.get())
		{
			error="Cannot preserve existing branches without the previous package";
			return false;
		}
		const string branchRoot=BranchRoot(identity.ActiveBranchID);
		string manifest=ManifestJSON(identity);
		if(manifest.empty()){error="Cannot serialize the package manifest";return false;}

		std::auto_ptr<Container::Writer> writer(m_Container.NewWriter());
		if(!writer->Open(path,error)) return false;
		if(source.get() &&
			(!CopyPreservedBranches(*writer,*source,identity.ActiveBranchID,error)||
			!CopyActiveExtras(*writer,*source,request.SourceBranchID,identity.ActiveBranchID,error)))
			return false;
		if(!writer->AddMemory(m_Layout.ManifestName,manifest,error)) return false;
		if (!m_Layout.MetadataName.empty()
			&& !writer->AddMemory(m_Layout.MetadataName, MetadataJSON(identity), error))
			return false;
		if(!payload.Write(*writer,branchRoot,error)) return false;
		return writer->Finish(path,error);
	}

	/* The branch folders a write would preserve from: the request's package
	   (extracted for the look) or workspace. The application asks before
	   deciding the identity, so folders the manifest never listed are kept too. */
	std::vector<std::string> Package::BranchesIn(const SaveRequest &request, string &error) const
	{
		Folder *sourceFolder=NULL;
		vector<string> ids;
		if(!OpenPreserveSource(request,sourceFolder,error))
			return ids;
		std::auto_ptr<Folder> source(sourceFolder);
		if(source.get()) ids=ListBranches(*source);
		return ids;
	}

	bool Package::Open(const string &path, const string &branchId,
		Folder &folder, Identity &identity, string &branchRoot, string &error) const
	{
		error.clear();
		if(path.empty()){error="No package filename";return false;}
		if(folder.IsOpen()){error="Package working folder is not empty";return false;}
		if(!folder.Create(m_Layout.WorkPrefix,error)) return false;
		Identity nextIdentity;string nextRoot;
		bool ok=m_Container.Extract(path,folder,error);
		if(ok)
		{
			if(!folder.IsFile(m_Layout.ManifestName)
				&& (m_Layout.LegacyManifestName.empty() || !folder.IsFile(m_Layout.LegacyManifestName)))
			{error="Not a package: no "+m_Layout.ManifestName+" in it";ok=false;}
			else
				ok=ReadManifest(folder,nextIdentity,nextRoot,error);
		}
		if(ok && !branchId.empty())
		{
			bool found=false;
			for(size_t i=0;i<nextIdentity.Branches.size();++i)
			{
				const Branch &point=nextIdentity.Branches[i];
				if(point.ID!=branchId) continue;
				nextIdentity.ActiveBranchID=point.ID;
				nextIdentity.ActiveBranchName=point.Name;
				nextRoot=BranchRoot(point.ID);
				found=true;
				break;
			}
			if(!found){error="Save point is no longer available";ok=false;}
		}
		if(ok && !folder.IsDirectory(nextRoot.substr(0,nextRoot.size()-1)))
		{error="Active branch has no folder in the package";ok=false;}
		if(!ok){folder.Remove();return false;}
		identity=nextIdentity;branchRoot=nextRoot;return true;
	}

	bool Package::WriteManifest(const string &path, const Identity &identity, string &error) const
	{
		error.clear();
		if(path.empty()){error="No package filename";return false;}
		if(identity.PackageID.empty()||identity.ActiveBranchID.empty())
		{error="WriteManifest requires package and branch identity";return false;}

		std::auto_ptr<Folder> folder(NewFolder());
		if(!folder->Create(m_Layout.WorkPrefix,error)||!m_Container.Extract(path,*folder,error))
			return false;

		string manifest=ManifestJSON(identity);
		if(manifest.empty()){error="Cannot serialize the package manifest";return false;}
		if(!folder->Write(m_Layout.ManifestName,manifest,error))
			return false;
		if (!m_Layout.MetadataName.empty()
			&& !folder->Write(m_Layout.MetadataName, MetadataJSON(identity), error))
			return false;
		if (!m_Layout.LegacyManifestName.empty())
			folder->RemoveEntry(m_Layout.LegacyManifestName);

		// Re-pack the whole tree so branch files are byte-copied.
		std::auto_ptr<Container::Writer> writer(m_Container.NewWriter());
		if(!writer->Open(path,error)) return false;
		if(!folder->ExportTree(*writer,string(),string(),error)) return false;
		return writer->Finish(path,error);
	}

} // namespace Spumoni
