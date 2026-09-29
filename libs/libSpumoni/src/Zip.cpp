// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Zip — a ZIP32 container: written entry by entry through a Writer,
// read by central directory, extracted into a folder. No knowledge of what
// the entries are; that belongs to the package on top.
// Code generated/modified by ChatGPT (GPT-5.6).
// Save-time named branches generated/modified by GROK (Grok Build).
// Lifted out of Spiral::File::Archive by Claude (moved from PatchArchiveContainer.h).

#include "Zip.h"
#include "Folder.h"

#include <zlib.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <limits>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

namespace Spumoni
{

	bool Zip::Write16(FILE *file, uint16_t value)
	{
		unsigned char b[2]={(unsigned char)value,(unsigned char)(value>>8)};
		return fwrite(b,1,2,file)==2;
	}

	bool Zip::Write32(FILE *file, uint32_t value)
	{
		unsigned char b[4]={(unsigned char)value,(unsigned char)(value>>8),
			(unsigned char)(value>>16),(unsigned char)(value>>24)};
		return fwrite(b,1,4,file)==4;
	}

	uint16_t Zip::Read16(const unsigned char *p)
	{
		return (uint16_t)(p[0]|((uint16_t)p[1]<<8));
	}

	uint32_t Zip::Read32(const unsigned char *p)
	{
		return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
			((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
	}

	bool Zip::Seek(FILE *file, uint64_t offset)
	{
		return offset<=kMaxArchiveSize && fseeko(file,(off_t)offset,SEEK_SET)==0;
	}

	bool Zip::Tell(FILE *file, uint32_t &offset)
	{
		off_t where=ftello(file);
		if(where<0 || (uint64_t)where>kMaxArchiveSize) return false;
		offset=(uint32_t)where;
		return true;
	}

	string Zip::ErrnoText(const string &what)
	{
		return what+": "+strerror(errno);
	}

	string Zip::BaseName(const string &path)
	{
		size_t end=path.size();
		while(end && path[end-1]=='/') --end;
		size_t slash=path.rfind('/',end ? end-1 : 0);
		return path.substr(slash==string::npos ? 0 : slash+1,
			end-(slash==string::npos ? 0 : slash+1));
	}

	string Zip::SafeName(const string &value, const string &fallback)
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

	bool Zip::SafeArchivePath(const string &name)
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

	Zip::Writer::Writer():m_File(NULL){}

	Zip::Writer::~Writer()
	{
	if(m_File) fclose(m_File);
	if(!m_Path.empty()) unlink(m_Path.c_str());
	}

	bool Zip::Writer::Open(const string &path, string &error)
	{
		// Modified by ChatGPT: atomic save with an exclusively created sibling.
		string pattern=path+".tmp-XXXXXX";
		vector<char> name(pattern.begin(),pattern.end()); name.push_back(0);
		int fd=mkstemp(&name[0]);
		if(fd<0){error=ErrnoText("Cannot create patch archive");return false;}
		m_Path=&name[0];
		m_File=fdopen(fd,"wb+");
		if(!m_File) close(fd);
		if(!m_File){error=ErrnoText("Cannot create patch archive");return false;}
		return true;
	}

	bool Zip::Writer::AddMemory(const string &name, const string &data, string &error)
	{
		return Add(name,(const unsigned char*)data.data(),data.size(),NULL,error);
	}

	bool Zip::Writer::AddFile(const string &name, const string &path, string &error)
	{
		FILE *input=fopen(path.c_str(),"rb");
		if(!input){error=ErrnoText("Cannot read asset "+path);return false;}
		bool ok=Add(name,NULL,0,input,error);
		fclose(input);
		return ok;
	}

	bool Zip::Writer::AddDirectory(const string &name, string &error)
	{
		string directory=name[name.size()-1]=='/' ? name : name+"/";
		if(!SafeArchivePath(directory) || directory.size()>65535 ||
			!m_Names.insert(directory).second)
		{error="Unsafe or duplicate archive directory: "+directory;return false;}
		Entry entry;entry.Name=directory;entry.Method=0;entry.CRC=0;
		entry.Compressed=entry.Size=0;
		if(!Tell(m_File,entry.Offset)||!Write32(m_File,0x04034b50)||
			!Write16(m_File,20)||!Write16(m_File,0)||!Write16(m_File,0)||
			!Write16(m_File,0)||!Write16(m_File,0)||!Write32(m_File,0)||
			!Write32(m_File,0)||!Write32(m_File,0)||
			!Write16(m_File,(uint16_t)directory.size())||!Write16(m_File,0)||
			fwrite(directory.data(),1,directory.size(),m_File)!=directory.size())
		{error="Error writing patch archive directory";return false;}
		m_Entries.push_back(entry);return true;
	}

	bool Zip::Writer::Finish(const string &path, string &error)
	{
		uint32_t centralOffset=0, centralEnd=0;
		if(!Tell(m_File,centralOffset)){error="Patch archive exceeds ZIP32 limits";return false;}
		for(size_t i=0;i<m_Entries.size();++i)
		{
			const Entry &e=m_Entries[i];
			if(!Write32(m_File,0x02014b50)||!Write16(m_File,20)||!Write16(m_File,20)||
				!Write16(m_File,0)||!Write16(m_File,e.Method)||!Write16(m_File,0)||
				!Write16(m_File,0)||!Write32(m_File,e.CRC)||!Write32(m_File,e.Compressed)||
				!Write32(m_File,e.Size)||!Write16(m_File,(uint16_t)e.Name.size())||
				!Write16(m_File,0)||!Write16(m_File,0)||!Write16(m_File,0)||
				!Write16(m_File,0)||!Write32(m_File,0)||!Write32(m_File,e.Offset)||
				fwrite(e.Name.data(),1,e.Name.size(),m_File)!=e.Name.size())
			{error="Error writing patch archive directory";return false;}
		}
		if(!Tell(m_File,centralEnd) || m_Entries.size()>65535)
		{error="Patch archive exceeds ZIP32 limits";return false;}
		uint32_t centralSize=centralEnd-centralOffset;
		if(!Write32(m_File,0x06054b50)||!Write16(m_File,0)||!Write16(m_File,0)||
			!Write16(m_File,(uint16_t)m_Entries.size())||
			!Write16(m_File,(uint16_t)m_Entries.size())||!Write32(m_File,centralSize)||
			!Write32(m_File,centralOffset)||!Write16(m_File,0)||fflush(m_File)!=0)
		{error="Error finalizing patch archive";return false;}
		if(fclose(m_File)!=0){m_File=NULL;error="Error closing patch archive";return false;}
		m_File=NULL;
		if(rename(m_Path.c_str(),path.c_str())!=0)
		{error=ErrnoText("Cannot replace patch archive");return false;}
		m_Path.clear();
		return true;
	}

	bool Zip::Writer::Add(const string &name, const unsigned char *memory, size_t memorySize,
		FILE *input, string &error)
	{
		if(!SafeArchivePath(name) || name.size()>65535)
		{error="Unsafe or overlong archive asset path";return false;}
		if(!m_Names.insert(name).second)
		{error="Two bundled assets map to the same archive path: "+name;return false;}
		Entry entry; entry.Name=name; entry.Method=8;
		if(!Tell(m_File,entry.Offset)){error="Patch archive exceeds ZIP32 limits";return false;}
		if(!Write32(m_File,0x04034b50)||!Write16(m_File,20)||!Write16(m_File,0)||
			!Write16(m_File,entry.Method)||!Write16(m_File,0)||!Write16(m_File,0)||
			!Write32(m_File,0)||!Write32(m_File,0)||!Write32(m_File,0)||
			!Write16(m_File,(uint16_t)name.size())||!Write16(m_File,0)||
			fwrite(name.data(),1,name.size(),m_File)!=name.size())
		{error="Error writing patch archive header";return false;}

		z_stream z; memset(&z,0,sizeof(z));
		if(deflateInit2(&z,Z_DEFAULT_COMPRESSION,Z_DEFLATED,-MAX_WBITS,8,
			Z_DEFAULT_STRATEGY)!=Z_OK){error="Cannot initialize ZIP compressor";return false;}
		unsigned char in[65536],out[65536];
		uint64_t total=0,compressed=0; uLong crc=crc32(0L,Z_NULL,0);
		size_t memoryOffset=0; int flush=Z_NO_FLUSH,ret=Z_OK;
		do
		{
			size_t n=0;
			if(input) n=fread(in,1,sizeof(in),input);
			else
			{
				n=min(sizeof(in),memorySize-memoryOffset);
				if(n) memcpy(in,memory+memoryOffset,n);
				memoryOffset+=n;
			}
			if(input && n<sizeof(in) && ferror(input))
			{deflateEnd(&z);error="Error reading asset";return false;}
			total+=n;
			if(total>kMaxEntrySize){deflateEnd(&z);error="Asset exceeds 2 GiB limit";return false;}
			crc=crc32(crc,in,(uInt)n);
			flush=(input ? feof(input) : memoryOffset==memorySize) ? Z_FINISH : Z_NO_FLUSH;
			z.next_in=in; z.avail_in=(uInt)n;
			do
			{
				z.next_out=out; z.avail_out=sizeof(out);
				ret=deflate(&z,flush);
				if(ret!=Z_OK && ret!=Z_STREAM_END)
				{deflateEnd(&z);error="Error compressing patch asset";return false;}
				size_t have=sizeof(out)-z.avail_out;
				if(have && fwrite(out,1,have,m_File)!=have)
				{deflateEnd(&z);error="Error writing patch asset";return false;}
				compressed+=have;
			}while(z.avail_out==0);
		}while(flush!=Z_FINISH || ret!=Z_STREAM_END);
		deflateEnd(&z);
		if(compressed>std::numeric_limits<uint32_t>::max() || total>std::numeric_limits<uint32_t>::max())
		{error="Patch asset exceeds ZIP32 limits";return false;}
		entry.CRC=(uint32_t)crc;entry.Compressed=(uint32_t)compressed;entry.Size=(uint32_t)total;
		uint32_t end;
		if(!Tell(m_File,end)||!Seek(m_File,(uint64_t)entry.Offset+14)||
			!Write32(m_File,entry.CRC)||!Write32(m_File,entry.Compressed)||
			!Write32(m_File,entry.Size)||!Seek(m_File,end))
		{error="Error updating patch archive header";return false;}
		m_Entries.push_back(entry);
		return true;
	}

	bool Zip::AddTree(Writer &zip, const string &diskPath,
		const string &archivePath, bool root, string &error)
	{
		struct stat st;
		if((root ? stat(diskPath.c_str(),&st) : lstat(diskPath.c_str(),&st))!=0)
		{error=ErrnoText("Cannot inspect asset "+diskPath);return false;}
		if(S_ISREG(st.st_mode)) return zip.AddFile(archivePath,diskPath,error);
		if(!S_ISDIR(st.st_mode))
		{if(root) error="Referenced asset is not a regular file or directory: "+diskPath;return !root;}
		DIR *dir=opendir(diskPath.c_str());
		if(!dir){error=ErrnoText("Cannot read asset directory "+diskPath);return false;}
		vector<string> names; struct dirent *item;
		while((item=readdir(dir))!=NULL)
			if(strcmp(item->d_name,".")&&strcmp(item->d_name,"..")) names.push_back(item->d_name);
		closedir(dir); sort(names.begin(),names.end());
		if(!zip.AddDirectory(archivePath,error)) return false;
		for(size_t i=0;i<names.size();++i)
			if(!AddTree(zip,diskPath+"/"+names[i],archivePath+"/"+SafeName(names[i],"asset"),false,error)) return false;
		return true;
	}


	bool Zip::AddTreeExact(Writer &zip, const string &diskPath,
		const string &archivePath, string &error)
	{
		struct stat st;
		if(lstat(diskPath.c_str(),&st)!=0)
		{error=ErrnoText("Cannot inspect "+diskPath);return false;}
		if(S_ISREG(st.st_mode)) return zip.AddFile(archivePath,diskPath,error);
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
		if(!zip.AddDirectory(archivePath,error)) return false;
		for(size_t i=0;i<names.size();++i)
		{
			string child=names[i];
			if(child.empty()||child=="."||child==".."||child.find('/')!=string::npos)
				continue;
			if(!AddTreeExact(zip,diskPath+"/"+child,archivePath+"/"+child,error))
				return false;
		}
		return true;
	}

	bool Zip::ExtractTo(const char *path, const string &folder, string &error)
	{
		FILE *file=fopen(path,"rb");
		if(!file){error=ErrnoText("Cannot open patch archive");return false;}
		vector<Entry> entries;
		if(!ReadCentralDirectory(file,entries,error)){fclose(file);return false;}
		for(size_t i=0;i<entries.size();++i)
			if(!ExtractEntry(file,entries[i],folder,error))
			{
				fclose(file);return false;
			}
		fclose(file);return true;
	}

	bool Zip::ReadCentralDirectory(FILE *file, vector<Entry> &entries, string &error)
	{
		if(fseeko(file,0,SEEK_END)!=0){error="Cannot seek in patch archive";return false;}
		off_t end=ftello(file);if(end<22){error="Patch archive is truncated";return false;}
		size_t tail=(size_t)min<off_t>(end,65557);vector<unsigned char> data(tail);
		if(fseeko(file,end-tail,SEEK_SET)!=0||fread(&data[0],1,tail,file)!=tail)
		{error="Cannot read patch archive directory";return false;}
		size_t pos=tail-22;bool found=false;
		for(;;)
		{
			if(Read32(&data[pos])==0x06054b50){found=true;break;}
			if(pos==0) break;
			--pos;
		}
		if(!found){error="Patch archive has no ZIP directory";return false;}
		uint16_t disk=Read16(&data[pos+4]),cdDisk=Read16(&data[pos+6]);
		uint16_t diskEntries=Read16(&data[pos+8]),count=Read16(&data[pos+10]);
		uint32_t cdSize=Read32(&data[pos+12]),cdOffset=Read32(&data[pos+16]);
		if(disk||cdDisk||diskEntries!=count||(uint64_t)cdOffset+cdSize>(uint64_t)end)
		{error="Multi-disk, ZIP64, or damaged .ssmp archives are not supported";return false;}
		if(!Seek(file,cdOffset)){error="Cannot seek to patch archive directory";return false;}
		set<string> names;
		uint64_t totalSize=0;
		for(unsigned int i=0;i<count;++i)
		{
			unsigned char h[46];if(fread(h,1,sizeof(h),file)!=sizeof(h)||Read32(h)!=0x02014b50)
			{error="Damaged patch archive directory";return false;}
			uint16_t flags=Read16(h+8),method=Read16(h+10),nameLen=Read16(h+28);
			uint16_t extraLen=Read16(h+30),commentLen=Read16(h+32);
			Entry e;e.Method=method;e.CRC=Read32(h+16);e.Compressed=Read32(h+20);
			e.Size=Read32(h+24);e.Offset=Read32(h+42);
			vector<char> name(nameLen);if(nameLen&&fread(&name[0],1,nameLen,file)!=nameLen)
			{error="Damaged patch archive filename";return false;}
			e.Name.assign(name.begin(),name.end());
			totalSize+=e.Size;
			if((flags&1)||!SafeArchivePath(e.Name)||(method!=0&&method!=8)||
				e.Size>kMaxEntrySize||totalSize>kMaxArchiveSize||
				!names.insert(e.Name).second)
			{error="Unsupported or unsafe entry in patch archive: "+e.Name;return false;}
			if(fseeko(file,(off_t)extraLen+commentLen,SEEK_CUR)!=0)
			{error="Damaged patch archive directory";return false;}
			entries.push_back(e);
		}
		return true;
	}

	bool Zip::ExtractEntry(FILE *archive, const Entry &entry,
		const string &workspace, string &error)
	{
		if(!Seek(archive,entry.Offset)){error="Invalid patch archive offset";return false;}
		unsigned char h[30];if(fread(h,1,sizeof(h),archive)!=sizeof(h)||Read32(h)!=0x04034b50)
		{error="Damaged patch archive entry";return false;}
		uint16_t nameLen=Read16(h+26),extraLen=Read16(h+28);
		uint64_t dataOffset=(uint64_t)entry.Offset+30+nameLen+extraLen;
		if(!Seek(archive,dataOffset)){error="Invalid patch archive data offset";return false;}
		string outputPath=workspace+"/"+entry.Name;
		if(!Folder::MakeDirectories(outputPath,error)) return false;
		if(entry.Name[entry.Name.size()-1]=='/')
		{
			if(entry.Size||entry.Compressed)
			{error="Invalid compressed directory entry: "+entry.Name;return false;}
			if(mkdir(outputPath.c_str(),0700)!=0&&errno!=EEXIST)
			{error=ErrnoText("Cannot extract directory "+entry.Name);return false;}
			return true;
		}
		FILE *output=fopen(outputPath.c_str(),"wb");
		if(!output){error=ErrnoText("Cannot extract "+entry.Name);return false;}
		unsigned char in[65536],out[65536];uint64_t remaining=entry.Compressed,total=0;
		uLong crc=crc32(0L,Z_NULL,0);bool ok=true;
		if(entry.Method==0)
		{
			while(remaining)
			{
				size_t n=(size_t)min<uint64_t>(remaining,sizeof(in));
				if(fread(in,1,n,archive)!=n||fwrite(in,1,n,output)!=n){ok=false;break;}
				crc=crc32(crc,in,(uInt)n);remaining-=n;total+=n;
			}
		}
		else
		{
			z_stream z;memset(&z,0,sizeof(z));if(inflateInit2(&z,-MAX_WBITS)!=Z_OK)ok=false;
			int ret=Z_OK;
			while(ok&&remaining&&ret!=Z_STREAM_END)
			{
				size_t n=(size_t)min<uint64_t>(remaining,sizeof(in));
				if(fread(in,1,n,archive)!=n){ok=false;break;}remaining-=n;
				z.next_in=in;z.avail_in=(uInt)n;
				while(ok&&z.avail_in)
				{
					z.next_out=out;z.avail_out=sizeof(out);ret=inflate(&z,Z_NO_FLUSH);
					if(ret!=Z_OK&&ret!=Z_STREAM_END){ok=false;break;}
					size_t have=sizeof(out)-z.avail_out;total+=have;
					if(total>entry.Size||fwrite(out,1,have,output)!=have){ok=false;break;}
					crc=crc32(crc,out,(uInt)have);
				}
			}
			if(ok&&(ret!=Z_STREAM_END||remaining!=0)) ok=false;
			inflateEnd(&z);
		}
		if(fclose(output)!=0)ok=false;
		if(!ok||total!=entry.Size||(uint32_t)crc!=entry.CRC)
		{unlink(outputPath.c_str());error="Damaged compressed data for "+entry.Name;return false;}
		return true;
	}

} // namespace Spumoni
