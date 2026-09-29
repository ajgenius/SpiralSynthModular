// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Package — the mechanics of the layered package: unpacking into a
// working folder, the manifest, carrying branch folders and extras forward,
// writing the active branch through the application's Payload.
// Grown out of Spiral::File::Archive, the SpiralSynthModular .ssmp package:
// Code generated/modified by ChatGPT (GPT-5.6).
// Save-time named branches generated/modified by GROK (Grok Build).

#include "Package.h"
#include "Folder.h"
#include "JSONParser.h"

#include <memory>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

using namespace std;

namespace Spumoni
{
	typedef Slick::JSONValue J;

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

	Package::Package(const Container &container, const Layout &layout, const Application &application)
		: m_Container(container)
		, m_Layout(layout)
		, m_Application(application)
	{
	}

	/* Branch folders present in a source package or workspace. */
	std::vector<std::string> Package::ListBranches(const string &sourceRoot)
	{
		vector<string> ids;
		if(sourceRoot.empty()) return ids;
		string branchesPath=sourceRoot+"/branches";
		vector<string> names;
		if(!Folder::List(branchesPath,names)) return ids;
		for(size_t i=0;i<names.size();++i)
		{
			const string &id=names[i];
			if(!IsUUID(id)) continue;
			if(Folder::IsDirectory(branchesPath+"/"+id))
				ids.push_back(id);
		}
		return ids;
	}

	bool Package::CopyPreservedBranches(Container::Writer &writer, const string &sourceRoot,
		const string &rewriteID, string &error) const
	{
		if(sourceRoot.empty()) return true;
		string branchesPath=sourceRoot+"/branches";
		vector<string> ids=ListBranches(sourceRoot);
		for(size_t i=0;i<ids.size();++i)
		{
			if(ids[i]==rewriteID) continue;
			if(!Container::AddTreeExact(writer,branchesPath+"/"+ids[i],"branches/"+ids[i],error))
				return false;
		}
		return true;
	}

	/* Anything in the source branch beside the application's payload files
	   carries over to the active branch as it is. */
	bool Package::CopyActiveExtras(Container::Writer &writer, const string &sourceRoot,
		const string &sourceID, const string &activeID, string &error) const
	{
		if(sourceRoot.empty()||sourceID.empty()) return true;
		string branchPath=sourceRoot+"/branches/"+sourceID;
		if(!Folder::IsDirectory(branchPath)) return true;
		vector<string> names;
		if(!Folder::List(branchPath,names)){error=Container::ErrnoText("Cannot preserve branch metadata");return false;}
		for(size_t i=0;i<names.size();++i)
		{
			const string &name=names[i];
			bool payload=false;
			for(size_t p=0;p<m_Layout.Payload.size();++p)
				if(m_Layout.Payload[p]==name){payload=true;break;}
			if(payload) continue;
			string disk=branchPath+"/"+name;
			string archive="branches/"+activeID+"/"+name;
			struct stat st;
			if(lstat(disk.c_str(),&st)!=0){error=Container::ErrnoText("Cannot inspect branch metadata");return false;}
			if(S_ISDIR(st.st_mode))
			{
				if(!Container::AddTreeExact(writer,disk,archive,error)) return false;
			}
			else if(S_ISREG(st.st_mode))
			{
				if(!writer.AddFile(archive,disk,error)) return false;
			}
		}
		return true;
	}

	bool Package::OpenPreserveSource(const SaveRequest &request,
		string &sourceRoot, Folder &ownedTemp, string &error) const
	{
		sourceRoot.clear();
		if(!request.ExistingPackage.empty())
		{
			if(Folder::IsFile(request.ExistingPackage))
			{
				if(!ownedTemp.Create(m_Layout.WorkPrefix,error)||
					!m_Container.Extract(request.ExistingPackage,ownedTemp.Path(),error))
					return false;
				sourceRoot=ownedTemp.Path();
				return true;
			}
		}
		if(!request.ExistingWorkspace.empty())
		{
			if(Folder::IsDirectory(request.ExistingWorkspace))
			{
				sourceRoot=request.ExistingWorkspace;
				return true;
			}
		}
		if(!request.ExistingPackage.empty() || !request.ExistingWorkspace.empty())
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
		Slick::JSONOwner root(J::MakeObject());
		root->Retain();
		root->Set("package",J::MakeString(Stamp()));
		m_Application.Describe(*root.get(),identity);
		root->Set("package_id",J::MakeString(identity.PackageID));
		root->Set("saved_at",J::MakeString(SaveTimeUTC()));
		root->Set("current_branch",J::MakeString(identity.ActiveBranchID));

		J *branchNames=J::MakeObject();
		J *branches=J::MakeArray();
		for(size_t i=0;i<identity.Branches.size();++i)
		{
			const Branch &item=identity.Branches[i];
			string name=item.Name;
			if(name.empty() && item.ID==identity.ActiveBranchID)
				name=identity.ActiveBranchName;
			branchNames->Set(item.ID,J::MakeString(name));
			J *branch=J::MakeObject();
			branch->Set("id",J::MakeString(item.ID));
			branch->Set("kind",J::MakeString(item.Kind.empty()?"named":item.Kind));
			branch->Set("path",J::MakeString(BranchRoot(item.ID)));
			if(item.ParentID.empty())
				branch->Set("parent_id",J::MakeNull());
			else
				branch->Set("parent_id",J::MakeString(item.ParentID));
			if(item.ForkSaveID.empty())
				branch->Set("fork_save_id",J::MakeNull());
			else
				branch->Set("fork_save_id",J::MakeString(item.ForkSaveID));
			branches->Append(branch);
		}
		J *names=J::MakeObject();
		names->Set("project",J::MakeString(identity.ActiveBranchName));
		names->Set("branches",branchNames);
		root->Set("names",names);
		root->Set("branches",branches);

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

	bool Package::ReadManifest(const string &path,
		Identity &identity, string &branchRoot, string &error) const
	{
		string parseError;
		Slick::JSONOwner storage(Slick::ParseJSON(path.c_str(),false,&parseError));
		if(!storage.get()){error=parseError.empty()?"Cannot read package manifest":parseError;return false;}
		const J &root=*storage.get();
		if(root.GetType()!=J::Object){error="Not a package manifest";return false;}

		// The package level first: a newer arrangement is refused whatever
		// the payload says. A manifest without the stamp predates it and is
		// Ver 1, the only arrangement there was.
		if(const J *package=Member(root,"package",J::String))
		{
			long found=0;
			Status status=Check(package->Text(),found);
			if(status==Newer||status==Foreign)
			{error=m_Layout.ManifestName+": "+Reason(status,found);return false;}
		}

		// Then the application's part: its own stamp and members.
		if(!m_Application.Accept(root,identity,error)) return false;

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
		string preserveRoot;
		Folder ownedTemp;
		if(!OpenPreserveSource(request,preserveRoot,ownedTemp,error))
			return false;
		bool needPreserve=false;
		for(size_t i=0;i<identity.Branches.size();++i)
			if(identity.Branches[i].ID!=identity.ActiveBranchID){needPreserve=true;break;}
		if(needPreserve && preserveRoot.empty())
		{
			error="Cannot preserve existing branches without the previous package";
			return false;
		}
		const string branchRoot=BranchRoot(identity.ActiveBranchID);
		string manifest=ManifestJSON(identity);
		if(manifest.empty()){error="Cannot serialize the package manifest";return false;}

		std::auto_ptr<Container::Writer> writer(m_Container.NewWriter());
		if(!writer->Open(path,error)) return false;
		if(!CopyPreservedBranches(*writer,preserveRoot,identity.ActiveBranchID,error)||
			!CopyActiveExtras(*writer,preserveRoot,request.SourceBranchID,identity.ActiveBranchID,error))
			return false;
		if(!writer->AddMemory(m_Layout.ManifestName,manifest,error)) return false;
		if(!payload.Write(*writer,branchRoot,error)) return false;
		return writer->Finish(path,error);
	}

	/* The branch folders a write would preserve from: the request's package
	   (extracted for the look) or workspace. The application asks before
	   deciding the identity, so folders the manifest never listed are kept too. */
	std::vector<std::string> Package::BranchesIn(const SaveRequest &request, string &error) const
	{
		string preserveRoot;
		Folder ownedTemp;
		vector<string> ids;
		if(!OpenPreserveSource(request,preserveRoot,ownedTemp,error))
			return ids;
		ids=ListBranches(preserveRoot);
		return ids;
	}

	bool Package::Open(const string &path, const string &branchId,
		Folder &folder, Identity &identity, string &branchRoot, string &error) const
	{
		error.clear();
		if(path.empty()){error="No package filename";return false;}
		if(!folder.Path().empty()){error="Package working folder is not empty";return false;}
		if(!folder.Create(m_Layout.WorkPrefix,error)) return false;
		const string workspace=folder.Path();
		Identity nextIdentity;string nextRoot;
		bool ok=m_Container.Extract(path,workspace,error);
		if(ok)
		{
			const string manifestPath=workspace+"/"+m_Layout.ManifestName;
			if(!Folder::IsFile(manifestPath))
			{error="Not a package: no "+m_Layout.ManifestName+" in it";ok=false;}
			else
				ok=ReadManifest(manifestPath,nextIdentity,nextRoot,error);
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
		if(ok && !Folder::IsDirectory(workspace+"/"+nextRoot))
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

		Folder folder;
		if(!folder.Create(m_Layout.WorkPrefix,error)||!m_Container.Extract(path,folder.Path(),error))
			return false;
		const string workspace=folder.Path();

		string manifest=ManifestJSON(identity);
		if(manifest.empty()){error="Cannot serialize the package manifest";return false;}
		if(!Folder::WriteFile(workspace+"/"+m_Layout.ManifestName,manifest,error))
			return false;

		// Re-pack the whole workspace so branch files are byte-copied.
		std::auto_ptr<Container::Writer> writer(m_Container.NewWriter());
		if(!writer->Open(path,error)) return false;

		vector<string> names;
		if(!Folder::List(workspace,names))
		{error=Container::ErrnoText("Cannot read package workspace");return false;}
		for(size_t i=0;i<names.size();++i)
		{
			string disk=workspace+"/"+names[i];
			struct stat st;
			if(lstat(disk.c_str(),&st)!=0)
			{error=Container::ErrnoText("Cannot inspect "+disk);return false;}
			if(S_ISREG(st.st_mode))
			{
				if(!writer->AddFile(names[i],disk,error)) return false;
			}
			else if(S_ISDIR(st.st_mode))
			{
				if(!Container::AddTreeExact(*writer,disk,names[i],error)) return false;
			}
		}
		return writer->Finish(path,error);
	}

} // namespace Spumoni
