// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Folder — a private working tree on disk.
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

	Folder::Folder():m_Owned(false)
	{
	}

	Folder::~Folder()
	{
		if(m_Owned) Remove();
	}

	string Folder::TemporaryRoot()
	{
		const char *env=getenv("TMPDIR");
		string root=env&&*env ? env : "/tmp";
		while(root.size()>1 && root[root.size()-1]=='/') root.erase(root.size()-1);
		return root;
	}

	bool Folder::Create(const string &prefix, string &error)
	{
		if(!m_Path.empty()){error="Folder already created";return false;}
		string pattern=TemporaryRoot()+"/"+(prefix.empty() ? string("spumoni-") : prefix)+"XXXXXX";
		vector<char> name(pattern.begin(),pattern.end()); name.push_back(0);
		char *made=mkdtemp(&name[0]);
		if(!made){error=ErrnoText("Cannot create working folder");return false;}
		m_Path=made; m_Owned=true;
		return true;
	}

	string Folder::Release()
	{
		m_Owned=false;
		return m_Path;
	}

	void Folder::RemoveReleased(string &path, const string &prefix)
	{
		string guard=TemporaryRoot()+"/"+prefix;
		if(!prefix.empty() && path.compare(0,guard.size(),guard)==0)
			RemoveTree(path);
		path.clear();
	}

	void Folder::Remove()
	{
		if(!m_Path.empty()) RemoveTree(m_Path);
		m_Path.clear(); m_Owned=false;
	}

	string Folder::ErrnoText(const string &what)
	{
		return what+": "+strerror(errno);
	}

	bool Folder::IsDirectory(const string &path)
	{
		struct stat st;
		return stat(path.c_str(),&st)==0 && S_ISDIR(st.st_mode);
	}

	bool Folder::IsFile(const string &path)
	{
		struct stat st;
		return stat(path.c_str(),&st)==0 && S_ISREG(st.st_mode);
	}

	// Sorted child names, "." and ".." left out. False when unreadable.
	bool Folder::List(const string &path, vector<string> &names)
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

	bool Folder::MakeDirectories(const string &path, string &error)
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

	bool Folder::RemoveTree(const string &path)
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

	bool Folder::WriteFile(const string &path, const string &data, string &error)
	{
		FILE *out=fopen(path.c_str(),"wb");
		if(!out){error=ErrnoText("Cannot write "+path);return false;}
		size_t wrote=fwrite(data.data(),1,data.size(),out);
		if(wrote!=data.size()||fclose(out)!=0){error="Error writing "+path;return false;}
		return true;
	}

} // namespace Spumoni
