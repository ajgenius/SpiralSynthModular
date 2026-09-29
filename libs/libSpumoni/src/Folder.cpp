// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Folder — the tree walkers over the interface, the plain path
// helpers, and DiskFolder, a private working tree on disk.
// Tree helpers lifted from Spiral::File::Archive::Zip (ChatGPT GPT-5.6).

#include "Folder.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

namespace Spumoni
{

	// ---- Path -----------------------------------------------------------

	string Path::TemporaryRoot()
	{
		const char *env=getenv("TMPDIR");
		string root=env&&*env ? env : "/tmp";
		while(root.size()>1 && root[root.size()-1]=='/') root.erase(root.size()-1);
		return root;
	}

	string Path::ErrnoText(const string &what)
	{
		return what+": "+strerror(errno);
	}

	bool Path::IsDirectory(const string &path)
	{
		struct stat st;
		return stat(path.c_str(),&st)==0 && S_ISDIR(st.st_mode);
	}

	bool Path::IsFile(const string &path)
	{
		struct stat st;
		return stat(path.c_str(),&st)==0 && S_ISREG(st.st_mode);
	}

	// Sorted child names, "." and ".." left out. False when unreadable.
	bool Path::List(const string &path, vector<string> &names)
	{
		names.clear();
		DIR *dir=opendir(path.c_str());
		if(!dir) return false;
		struct dirent *item;
		while((item=readdir(dir))!=NULL)
			if(strcmp(item->d_name,".")&&strcmp(item->d_name,"..")) names.push_back(item->d_name);
		closedir(dir); sort(names.begin(),names.end());
		return true;
	}

	bool Path::MakeDirectories(const string &path, string &error)
	{
		for(size_t slash=1;(slash=path.find('/',slash))!=string::npos;++slash)
		{
			string dir=path.substr(0,slash);
			struct stat st;
			if(stat(dir.c_str(),&st)==0)
			{
				if(!S_ISDIR(st.st_mode)){error="Archive path is not a directory: "+dir;return false;}
			}
			else if(errno!=ENOENT || mkdir(dir.c_str(),0700)!=0)
			{
				error=ErrnoText("Cannot create "+dir); return false;
			}
		}
		return true;
	}

	bool Path::RemoveTree(const string &path)
	{
		struct stat st;if(lstat(path.c_str(),&st)!=0) return errno==ENOENT;
		if(S_ISDIR(st.st_mode))
		{
			DIR *dir=opendir(path.c_str());if(!dir)return false;struct dirent *item;bool ok=true;
			while((item=readdir(dir))!=NULL) if(strcmp(item->d_name,".")&&strcmp(item->d_name,".."))
				ok=RemoveTree(path+"/"+item->d_name)&&ok;
			closedir(dir);return rmdir(path.c_str())==0&&ok;
		}
		return unlink(path.c_str())==0;
	}

	bool Path::ReadFile(const string &path, string &data, string &error)
	{
		FILE *in=fopen(path.c_str(),"rb");
		if(!in){error=ErrnoText("Cannot read "+path);return false;}
		data.clear();
		char buffer[65536];
		for(;;)
		{
			size_t n=fread(buffer,1,sizeof(buffer),in);
			data.append(buffer,n);
			if(n<sizeof(buffer)) break;
		}
		bool ok=!ferror(in);
		fclose(in);
		if(!ok){error="Error reading "+path;return false;}
		return true;
	}

	bool Path::WriteFile(const string &path, const string &data, string &error)
	{
		FILE *out=fopen(path.c_str(),"wb");
		if(!out){error=ErrnoText("Cannot write "+path);return false;}
		size_t wrote=fwrite(data.data(),1,data.size(),out);
		if(wrote!=data.size()||fclose(out)!=0){error="Error writing "+path;return false;}
		return true;
	}

	string Path::Join(const string &root, const string &relative)
	{
		if(relative.empty()) return root;
		if(root.empty()) return relative;
		return root+"/"+relative;
	}

	unsigned long long Path::FileBytes(const string &path)
	{
		struct stat st;
		return stat(path.c_str(),&st)==0 && S_ISREG(st.st_mode) ? (unsigned long long)st.st_size : 0;
	}

	unsigned long long Path::TreeBytes(const string &path)
	{
		struct stat st;
		if(lstat(path.c_str(),&st)!=0) return 0;
		if(S_ISREG(st.st_mode)) return (unsigned long long)st.st_size;
		if(!S_ISDIR(st.st_mode)) return 0;
		vector<string> names;
		if(!List(path,names)) return 0;
		unsigned long long total=0;
		for(size_t i=0;i<names.size();++i) total+=TreeBytes(path+"/"+names[i]);
		return total;
	}

	// ---- Folder: the walkers -------------------------------------------

	// A disk tree's children into this folder under `relative`, streamed,
	// names kept. Symlinks and specials are skipped, as the container walkers
	// skip them.
	bool Folder::ImportTree(const string &diskPath, const string &relative, string &error)
	{
		vector<string> names;
		if(!Path::List(diskPath,names)){error=Path::ErrnoText("Cannot read "+diskPath);return false;}
		if(!relative.empty() && !MakeDirectory(relative,error)) return false;
		for(size_t i=0;i<names.size();++i)
		{
			string source=diskPath+"/"+names[i];
			string target=Path::Join(relative,names[i]);
			struct stat st;
			if(lstat(source.c_str(),&st)!=0){error=Path::ErrnoText("Cannot inspect "+source);return false;}
			if(S_ISDIR(st.st_mode))
			{
				if(!ImportTree(source,target,error)) return false;
			}
			else if(S_ISREG(st.st_mode))
			{
				FILE *in=fopen(source.c_str(),"rb");
				if(!in){error=Path::ErrnoText("Cannot read "+source);return false;}
				FILE *out=OpenWrite(target,error);
				if(!out){fclose(in);return false;}
				char buffer[65536];bool ok=true;
				for(;;)
				{
					size_t n=fread(buffer,1,sizeof(buffer),in);
					if(n && fwrite(buffer,1,n,out)!=n){ok=false;break;}
					if(n<sizeof(buffer)){ok=!ferror(in);break;}
				}
				fclose(in);
				if(!CloseWrite(out,error)) return false;
				if(!ok){error="Error copying "+source;return false;}
			}
		}
		return true;
	}

	// This folder's tree under `relative` into a Writer at `archivePath`,
	// exact names, directories included.
	bool Folder::ExportTree(Container::Writer &writer, const string &relative,
		const string &archivePath, string &error) const
	{
		if(IsFile(relative)) return AddTo(writer,relative,archivePath,error);
		if(!IsDirectory(relative)) return true;
		vector<string> names;
		if(!List(relative,names)){error="Cannot read "+relative;return false;}
		if(!archivePath.empty() && !writer.AddDirectory(archivePath,error)) return false;
		for(size_t i=0;i<names.size();++i)
			if(!ExportTree(writer,Path::Join(relative,names[i]),Path::Join(archivePath,names[i]),error))
				return false;
		return true;
	}

	// ---- DiskFolder -------------------------------------------------------

	DiskFolder::DiskFolder():m_Owned(false)
	{
	}

	DiskFolder::~DiskFolder()
	{
		if(m_Owned) Remove();
	}

	DiskFolder *DiskFolder::Adopt(const string &path)
	{
		DiskFolder *folder=new DiskFolder;
		folder->m_Path=path;
		while(folder->m_Path.size()>1 && folder->m_Path[folder->m_Path.size()-1]=='/')
			folder->m_Path.erase(folder->m_Path.size()-1);
		folder->m_Owned=false;
		return folder;
	}

	bool DiskFolder::Create(const string &prefix, string &error)
	{
		if(!m_Path.empty()){error="Folder already created";return false;}
		string pattern=Path::TemporaryRoot()+"/"+(prefix.empty() ? string("spumoni-") : prefix)+"XXXXXX";
		vector<char> name(pattern.begin(),pattern.end()); name.push_back(0);
		char *made=mkdtemp(&name[0]);
		if(!made){error=Path::ErrnoText("Cannot create working folder");return false;}
		m_Path=made; m_Owned=true;
		return true;
	}

	string DiskFolder::Release()
	{
		m_Owned=false;
		return m_Path;
	}

	void DiskFolder::RemoveReleased(string &path, const string &prefix)
	{
		string guard=Path::TemporaryRoot()+"/"+prefix;
		if(!prefix.empty() && path.compare(0,guard.size(),guard)==0)
			Path::RemoveTree(path);
		path.clear();
	}

	void DiskFolder::Remove()
	{
		if(m_Owned && !m_Path.empty()) Path::RemoveTree(m_Path);
		m_Path.clear(); m_Owned=false;
	}

	bool DiskFolder::IsDirectory(const string &relative) const
	{
		return !m_Path.empty() && Path::IsDirectory(Path::Join(m_Path,relative));
	}

	bool DiskFolder::IsFile(const string &relative) const
	{
		return !m_Path.empty() && Path::IsFile(Path::Join(m_Path,relative));
	}

	bool DiskFolder::List(const string &relative, vector<string> &names) const
	{
		return !m_Path.empty() && Path::List(Path::Join(m_Path,relative),names);
	}

	bool DiskFolder::MakeDirectory(const string &relative, string &error)
	{
		string full=Path::Join(m_Path,relative);
		if(!Path::MakeDirectories(full+"/",error)) return false;
		return true;
	}

	bool DiskFolder::Read(const string &relative, string &data, string &error) const
	{
		return Path::ReadFile(Path::Join(m_Path,relative),data,error);
	}

	bool DiskFolder::Write(const string &relative, const string &data, string &error)
	{
		string full=Path::Join(m_Path,relative);
		return Path::MakeDirectories(full,error) && Path::WriteFile(full,data,error);
	}

	bool DiskFolder::RemoveEntry(const string &relative)
	{
		return !relative.empty() && Path::RemoveTree(Path::Join(m_Path,relative));
	}

	FILE *DiskFolder::OpenWrite(const string &relative, string &error)
	{
		string full=Path::Join(m_Path,relative);
		if(!Path::MakeDirectories(full,error)) return NULL;
		FILE *file=fopen(full.c_str(),"wb");
		if(!file) error=Path::ErrnoText("Cannot write "+full);
		return file;
	}

	bool DiskFolder::CloseWrite(FILE *file, string &error)
	{
		if(fclose(file)!=0){error="Error writing file";return false;}
		return true;
	}

	FILE *DiskFolder::OpenRead(const string &relative, string &error) const
	{
		string full=Path::Join(m_Path,relative);
		FILE *file=fopen(full.c_str(),"rb");
		if(!file) error=Path::ErrnoText("Cannot read "+full);
		return file;
	}

	bool DiskFolder::PathFor(const string &relative, string &path, string &error)
	{
		string full=Path::Join(m_Path,relative);
		struct stat st;
		if(stat(full.c_str(),&st)!=0){error=Path::ErrnoText("Missing "+relative);return false;}
		path=full;
		return true;
	}

	bool DiskFolder::AddTo(Container::Writer &writer, const string &relative,
		const string &archiveName, string &error) const
	{
		return writer.AddFile(archiveName,Path::Join(m_Path,relative),error);
	}

} // namespace Spumoni
