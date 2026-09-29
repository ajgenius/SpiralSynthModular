// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Directory — a bare folder as the container.

#include "Directory.h"
#include "Folder.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

using namespace std;

namespace Spumoni
{

	const char *Directory::Kind() const
	{
		return "directory";
	}

	Container::Writer *Directory::NewWriter() const
	{
		return new Writer;
	}

	bool Directory::Extract(const string &path, const string &folder, string &error) const
	{
		if(!Folder::IsDirectory(path)){error="Not a package folder: "+path;return false;}
		return CopyTree(path,folder,error);
	}

	bool Directory::CopyFile(const string &from, const string &to, string &error)
	{
		FILE *in=fopen(from.c_str(),"rb");
		if(!in){error=ErrnoText("Cannot read "+from);return false;}
		FILE *out=fopen(to.c_str(),"wb");
		if(!out){fclose(in);error=ErrnoText("Cannot write "+to);return false;}
		char buffer[65536];bool ok=true;
		for(;;)
		{
			size_t n=fread(buffer,1,sizeof(buffer),in);
			if(n && fwrite(buffer,1,n,out)!=n){ok=false;break;}
			if(n<sizeof(buffer)){ok=!ferror(in);break;}
		}
		fclose(in);
		if(fclose(out)!=0) ok=false;
		if(!ok){unlink(to.c_str());error="Error copying "+from;return false;}
		return true;
	}

	// The children of `from` into `to`, which must exist. Symlinks and
	// specials are skipped, as the ZIP walker skips them.
	bool Directory::CopyTree(const string &from, const string &to, string &error)
	{
		vector<string> names;
		if(!Folder::List(from,names)){error=ErrnoText("Cannot read "+from);return false;}
		for(size_t i=0;i<names.size();++i)
		{
			string source=from+"/"+names[i],target=to+"/"+names[i];
			struct stat st;
			if(lstat(source.c_str(),&st)!=0){error=ErrnoText("Cannot inspect "+source);return false;}
			if(S_ISDIR(st.st_mode))
			{
				if(mkdir(target.c_str(),0700)!=0 && errno!=EEXIST)
				{error=ErrnoText("Cannot create "+target);return false;}
				if(!CopyTree(source,target,error)) return false;
			}
			else if(S_ISREG(st.st_mode))
			{
				if(!CopyFile(source,target,error)) return false;
			}
		}
		return true;
	}

	Directory::Writer::Writer()
	{
	}

	Directory::Writer::~Writer()
	{
		if(!m_Path.empty()) Folder::RemoveTree(m_Path);
	}

	// Build beside the target, swap in on Finish.
	bool Directory::Writer::Open(const string &path, string &error)
	{
		string pattern=path+".tmp-XXXXXX";
		vector<char> name(pattern.begin(),pattern.end()); name.push_back(0);
		if(!mkdtemp(&name[0])){error=ErrnoText("Cannot create package folder");return false;}
		m_Path=&name[0];
		return true;
	}

	bool Directory::Writer::Place(const string &name, string &target, string &error)
	{
		if(!SafeArchivePath(name)){error="Unsafe or overlong archive asset path";return false;}
		if(!m_Names.insert(name).second)
		{error="Two bundled assets map to the same archive path: "+name;return false;}
		target=m_Path+"/"+name;
		return Folder::MakeDirectories(target,error);
	}

	bool Directory::Writer::AddMemory(const string &name, const string &data, string &error)
	{
		string target;
		return Place(name,target,error) && Folder::WriteFile(target,data,error);
	}

	bool Directory::Writer::AddFile(const string &name, const string &path, string &error)
	{
		string target;
		return Place(name,target,error) && CopyFile(path,target,error);
	}

	bool Directory::Writer::AddDirectory(const string &name, string &error)
	{
		string directory=name[name.size()-1]=='/' ? name : name+"/";
		string target;
		if(!Place(directory,target,error)) return false;
		if(mkdir(target.c_str(),0700)!=0 && errno!=EEXIST)
		{error=ErrnoText("Cannot create "+target);return false;}
		return true;
	}

	bool Directory::Writer::Finish(const string &path, string &error)
	{
		string previous;
		if(Folder::IsDirectory(path))
		{
			string pattern=path+".old-XXXXXX";
			vector<char> name(pattern.begin(),pattern.end()); name.push_back(0);
			if(!mkdtemp(&name[0])){error=ErrnoText("Cannot set aside the previous package");return false;}
			previous=&name[0];
			rmdir(previous.c_str());
			if(rename(path.c_str(),previous.c_str())!=0)
			{error=ErrnoText("Cannot set aside the previous package");return false;}
		}
		if(rename(m_Path.c_str(),path.c_str())!=0)
		{
			error=ErrnoText("Cannot replace package folder");
			if(!previous.empty()) rename(previous.c_str(),path.c_str());
			return false;
		}
		m_Path.clear();
		if(!previous.empty()) Folder::RemoveTree(previous);
		return true;
	}

} // namespace Spumoni
