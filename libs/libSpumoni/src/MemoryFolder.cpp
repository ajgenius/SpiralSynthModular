// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::MemoryFolder — a working tree held entirely in memory.

#include "MemoryFolder.h"

#include <memory>

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;

namespace Spumoni
{

	MemoryFolder::MemoryFolder()
		: m_Open(false), m_Scratch(NULL), m_Bytes(0)
	{
	}

	MemoryFolder::~MemoryFolder()
	{
		Remove();
	}

	bool MemoryFolder::Create(const string &prefix, string &error)
	{
		if(m_Open){error="Folder already created";return false;}
		m_Prefix=prefix.empty() ? string("spumoni-") : prefix;
		m_Open=true;
		return true;
	}

	void MemoryFolder::Remove()
	{
		for(map<FILE *,Pending *>::iterator i=m_Pending.begin();i!=m_Pending.end();++i)
		{
			fclose(i->first);
			free(i->second->Buffer);
			delete i->second;
		}
		m_Pending.clear();
		m_Entries.clear();
		m_Bytes=0;
		delete m_Scratch; m_Scratch=NULL;
		m_Open=false;
	}

	// "a//b/", "./a" and "" all name what they look like; ".." never.
	string MemoryFolder::Normalise(const string &relative)
	{
		string result;
		size_t start=0;
		while(start<=relative.size())
		{
			size_t slash=relative.find('/',start);
			if(slash==string::npos) slash=relative.size();
			string part=relative.substr(start,slash-start);
			if(!part.empty() && part!=".")
			{
				if(!result.empty()) result+='/';
				result+=part;
			}
			start=slash+1;
		}
		return result;
	}

	bool MemoryFolder::ValidRelative(const string &relative)
	{
		size_t start=0;
		while(start<=relative.size())
		{
			size_t slash=relative.find('/',start);
			if(slash==string::npos) slash=relative.size();
			if(relative.compare(start,slash-start,"..")==0) return false;
			start=slash+1;
		}
		return true;
	}

	bool MemoryFolder::MakeParents(const string &relative, string &error)
	{
		size_t slash=0;
		while((slash=relative.find('/',slash))!=string::npos)
		{
			string parent=relative.substr(0,slash);
			map<string,Entry>::iterator found=m_Entries.find(parent);
			if(found==m_Entries.end())
			{
				Entry directory; directory.Directory=true;
				m_Entries[parent]=directory;
			}
			else if(!found->second.Directory)
			{error="Not a directory: "+parent;return false;}
			++slash;
		}
		return true;
	}

	void MemoryFolder::Forget(const string &relative)
	{
		map<string,Entry>::iterator found=m_Entries.find(relative);
		if(found==m_Entries.end()) return;
		m_Bytes-=found->second.Data.size();
		if(!found->second.Materialised.empty() && m_Scratch)
			m_Scratch->RemoveEntry(found->second.Materialised);
		m_Entries.erase(found);
	}

	bool MemoryFolder::IsDirectory(const string &relative) const
	{
		if(!m_Open) return false;
		string key=Normalise(relative);
		if(key.empty()) return true;
		map<string,Entry>::const_iterator found=m_Entries.find(key);
		return found!=m_Entries.end() && found->second.Directory;
	}

	bool MemoryFolder::IsFile(const string &relative) const
	{
		if(!m_Open) return false;
		map<string,Entry>::const_iterator found=m_Entries.find(Normalise(relative));
		return found!=m_Entries.end() && !found->second.Directory;
	}

	bool MemoryFolder::List(const string &relative, vector<string> &names) const
	{
		names.clear();
		if(!IsDirectory(relative)) return false;
		string prefix=Normalise(relative);
		if(!prefix.empty()) prefix+='/';
		for(map<string,Entry>::const_iterator i=m_Entries.lower_bound(prefix);i!=m_Entries.end();++i)
		{
			if(i->first.compare(0,prefix.size(),prefix)!=0) break;
			string rest=i->first.substr(prefix.size());
			if(rest.find('/')==string::npos) names.push_back(rest);
		}
		return true;
	}

	bool MemoryFolder::MakeDirectory(const string &relative, string &error)
	{
		if(!m_Open){error="Folder not created";return false;}
		string key=Normalise(relative);
		if(!ValidRelative(key)){error="Unsafe path: "+relative;return false;}
		if(key.empty()) return true;
		if(!MakeParents(key,error)) return false;
		map<string,Entry>::iterator found=m_Entries.find(key);
		if(found!=m_Entries.end())
		{
			if(found->second.Directory) return true;
			error="Not a directory: "+key;return false;
		}
		Entry directory; directory.Directory=true;
		m_Entries[key]=directory;
		return true;
	}

	bool MemoryFolder::Read(const string &relative, string &data, string &error) const
	{
		map<string,Entry>::const_iterator found=m_Entries.find(Normalise(relative));
		if(found==m_Entries.end()||found->second.Directory){error="Missing "+relative;return false;}
		data=found->second.Data;
		return true;
	}

	bool MemoryFolder::Write(const string &relative, const string &data, string &error)
	{
		if(!m_Open){error="Folder not created";return false;}
		string key=Normalise(relative);
		if(key.empty()||!ValidRelative(key)){error="Unsafe path: "+relative;return false;}
		if(!MakeParents(key,error)) return false;
		map<string,Entry>::iterator found=m_Entries.find(key);
		if(found!=m_Entries.end() && found->second.Directory){error="Is a directory: "+key;return false;}
		Forget(key);
		Entry file; file.Data=data;
		m_Entries[key]=file;
		m_Bytes+=data.size();
		return true;
	}

	bool MemoryFolder::RemoveEntry(const string &relative)
	{
		string key=Normalise(relative);
		if(key.empty()) return false;
		map<string,Entry>::iterator found=m_Entries.find(key);
		if(found==m_Entries.end()) return true;
		if(found->second.Directory)
		{
			string prefix=key+"/";
			for(map<string,Entry>::iterator i=m_Entries.lower_bound(prefix);
				i!=m_Entries.end() && i->first.compare(0,prefix.size(),prefix)==0;)
			{
				string child=i->first; ++i;
				Forget(child);
			}
		}
		Forget(key);
		return true;
	}

	// A memory stream; the entry appears when CloseWrite lands the bytes.
	FILE *MemoryFolder::OpenWrite(const string &relative, string &error)
	{
		if(!m_Open){error="Folder not created";return NULL;}
		string key=Normalise(relative);
		if(key.empty()||!ValidRelative(key)){error="Unsafe path: "+relative;return NULL;}
		if(!MakeParents(key,error)) return NULL;
		// open_memstream keeps the addresses it is given until fclose, so the
		// record must live on the heap, not in a local or a map node that moves.
		Pending *pending=new Pending;
		pending->Relative=key; pending->Buffer=NULL; pending->Size=0;
		FILE *file=open_memstream(&pending->Buffer,&pending->Size);
		if(!file){delete pending;error="Cannot open memory stream";return NULL;}
		m_Pending[file]=pending;
		return file;
	}

	bool MemoryFolder::CloseWrite(FILE *file, string &error)
	{
		map<FILE *,Pending *>::iterator found=m_Pending.find(file);
		if(found==m_Pending.end()){error="Not a stream of this folder";return false;}
		std::auto_ptr<Pending> pending(found->second);
		m_Pending.erase(found);
		// The buffer and size are final only after fclose.
		bool closed=fclose(file)==0;
		string data;
		if(closed && pending->Buffer) data.assign(pending->Buffer,pending->Size);
		free(pending->Buffer);
		if(!closed){error="Error closing memory stream";return false;}
		return Write(pending->Relative,data,error);
	}

	FILE *MemoryFolder::OpenRead(const string &relative, string &error) const
	{
		map<string,Entry>::const_iterator found=m_Entries.find(Normalise(relative));
		if(found==m_Entries.end()||found->second.Directory){error="Missing "+relative;return NULL;}
		const string &data=found->second.Data;
		FILE *file=data.empty() ? NULL : fmemopen((void*)data.data(),data.size(),"rb");
		if(!file) file=tmpfile();	// an empty entry: any empty stream will do
		if(!file) error="Cannot open memory stream";
		return file;
	}

	// The one way out: that entry alone, into a private disk scratch.
	bool MemoryFolder::PathFor(const string &relative, string &path, string &error)
	{
		string key=Normalise(relative);
		map<string,Entry>::iterator found=m_Entries.find(key);
		if(found==m_Entries.end()||found->second.Directory){error="Missing "+relative;return false;}
		if(!m_Scratch)
		{
			m_Scratch=new DiskFolder;
			if(!m_Scratch->Create(m_Prefix+"mem-",error)){delete m_Scratch;m_Scratch=NULL;return false;}
		}
		if(found->second.Materialised.empty())
		{
			if(!m_Scratch->Write(key,found->second.Data,error)) return false;
			found->second.Materialised=key;
		}
		return m_Scratch->PathFor(key,path,error);
	}

	bool MemoryFolder::AddTo(Container::Writer &writer, const string &relative,
		const string &archiveName, string &error) const
	{
		map<string,Entry>::const_iterator found=m_Entries.find(Normalise(relative));
		if(found==m_Entries.end()||found->second.Directory){error="Missing "+relative;return false;}
		return writer.AddMemory(archiveName,found->second.Data,error);
	}

} // namespace Spumoni
