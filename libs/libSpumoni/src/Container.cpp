// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Container — what every container shares: the entry-name rules and
// walking a disk tree into a Writer. Lifted from Spiral::File::Archive::Zip
// (ChatGPT GPT-5.6).

#include "Container.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

using namespace std;

namespace Spumoni
{

	string Container::ErrnoText(const string &what)
	{
		return what+": "+strerror(errno);
	}

	string Container::BaseName(const string &path)
	{
		size_t end=path.size();
		while(end && path[end-1]=='/') --end;
		size_t slash=path.rfind('/',end ? end-1 : 0);
		return path.substr(slash==string::npos ? 0 : slash+1,
			end-(slash==string::npos ? 0 : slash+1));
	}

	string Container::SafeName(const string &value, const string &fallback)
	{
		string result;
		for(size_t i=0;i<value.size();++i)
		{
			unsigned char c=(unsigned char)value[i];
			if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||
				c=='-'||c=='_'||c=='.') result+=(char)c;
			else result+='_';
		}
		while(!result.empty() && result[0]=='.') result.erase(0,1);
		return result.empty() ? fallback : result;
	}

	bool Container::SafeArchivePath(const string &name)
	{
		if(name.empty() || name[0]=='/' || name.find('\\')!=string::npos ||
			name.find('\0')!=string::npos) return false;
		size_t length=name.size();
		if(name[length-1]=='/') --length;
		if(!length) return false;
		size_t start=0;
		while(start<length)
		{
			size_t slash=name.find('/',start);
			if(slash>=length) slash=string::npos;
			string part=name.substr(start,slash==string::npos ? length-start : slash-start);
			if(part.empty() || part=="." || part=="..") return false;
			if(slash==string::npos) break;
			start=slash+1;
		}
		return true;
	}

	bool Container::AddTree(Writer &writer, const string &diskPath,
		const string &archivePath, bool root, string &error)
	{
		struct stat st;
		if((root ? stat(diskPath.c_str(),&st) : lstat(diskPath.c_str(),&st))!=0)
		{error=ErrnoText("Cannot inspect asset "+diskPath);return false;}
		if(S_ISREG(st.st_mode)) return writer.AddFile(archivePath,diskPath,error);
		if(!S_ISDIR(st.st_mode))
		{if(root) error="Referenced asset is not a regular file or directory: "+diskPath;return !root;}
		DIR *dir=opendir(diskPath.c_str());
		if(!dir){error=ErrnoText("Cannot read asset directory "+diskPath);return false;}
		vector<string> names; struct dirent *item;
		while((item=readdir(dir))!=NULL)
			if(strcmp(item->d_name,".")&&strcmp(item->d_name,"..")) names.push_back(item->d_name);
		closedir(dir); sort(names.begin(),names.end());
		if(!writer.AddDirectory(archivePath,error)) return false;
		for(size_t i=0;i<names.size();++i)
			if(!AddTree(writer,diskPath+"/"+names[i],archivePath+"/"+SafeName(names[i],"asset"),false,error)) return false;
		return true;
	}


	bool Container::AddTreeExact(Writer &writer, const string &diskPath,
		const string &archivePath, string &error)
	{
		struct stat st;
		if(lstat(diskPath.c_str(),&st)!=0)
		{error=ErrnoText("Cannot inspect "+diskPath);return false;}
		if(S_ISREG(st.st_mode)) return writer.AddFile(archivePath,diskPath,error);
		if(!S_ISDIR(st.st_mode)) return true;
		if(!SafeArchivePath(archivePath[archivePath.size()-1]=='/'?archivePath:archivePath+"/"))
		{error="Unsafe archive path: "+archivePath;return false;}
		DIR *dir=opendir(diskPath.c_str());
		if(!dir){error=ErrnoText("Cannot read "+diskPath);return false;}
		vector<string> names; struct dirent *item;
		while((item=readdir(dir))!=NULL)
			if(strcmp(item->d_name,".")&&strcmp(item->d_name,".."))
				names.push_back(item->d_name);
		closedir(dir); sort(names.begin(),names.end());
		if(!writer.AddDirectory(archivePath,error)) return false;
		for(size_t i=0;i<names.size();++i)
		{
			string child=names[i];
			if(child.empty()||child=="."||child==".."||child.find('/')!=string::npos)
				continue;
			if(!AddTreeExact(writer,diskPath+"/"+child,archivePath+"/"+child,error))
				return false;
		}
		return true;
	}

} // namespace Spumoni
