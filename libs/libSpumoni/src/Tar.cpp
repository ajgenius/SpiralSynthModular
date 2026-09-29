// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Tar — a ustar tarball as the container.

#include "Tar.h"
#include "Folder.h"

#include <zlib.h>

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
	namespace
	{
		const size_t kBlock = 512;

		// ustar header layout.
		struct Header
		{
			char name[100];
			char mode[8];
			char uid[8];
			char gid[8];
			char size[12];
			char mtime[12];
			char chksum[8];
			char typeflag;
			char linkname[100];
			char magic[6];
			char version[2];
			char uname[32];
			char gname[32];
			char devmajor[8];
			char devminor[8];
			char prefix[155];
			char pad[12];
		};

		void Octal(char *field, size_t width, unsigned long long value)
		{
			// width-1 digits and a NUL, as ustar wants.
			for(size_t i=width-1;i-->0;)
			{
				field[i]=(char)('0'+(value&7));
				value>>=3;
			}
			field[width-1]='\0';
		}

		unsigned long long FromOctal(const char *field, size_t width)
		{
			unsigned long long value=0;
			for(size_t i=0;i<width && field[i];++i)
			{
				if(field[i]==' ') continue;
				if(field[i]<'0'||field[i]>'7') break;
				value=(value<<3)+(unsigned long long)(field[i]-'0');
			}
			return value;
		}

		unsigned int Checksum(const Header &header)
		{
			const unsigned char *bytes=(const unsigned char*)&header;
			unsigned int sum=0;
			for(size_t i=0;i<kBlock;++i)
				sum+=(i>=148&&i<156) ? (unsigned int)' ' : bytes[i];
			return sum;
		}

		bool HasSuffix(const string &value, const char *suffix)
		{
			size_t n=strlen(suffix);
			if(value.size()<n) return false;
			for(size_t i=0;i<n;++i)
			{
				char c=value[value.size()-n+i];
				if(c>='A'&&c<='Z') c=(char)(c-'A'+'a');
				if(c!=suffix[i]) return false;
			}
			return true;
		}
	}

	bool Tar::Compressed(const string &path)
	{
		return HasSuffix(path,".tgz")||HasSuffix(path,".tar.gz")||HasSuffix(path,".gz");
	}

	const char *Tar::Kind() const
	{
		return "tar";
	}

	Container::Writer *Tar::NewWriter() const
	{
		return new Writer;
	}

	// gzread reads plain files transparently, so one reader serves both.
	bool Tar::Extract(const string &path, Folder &folder, string &error) const
	{
		gzFile file=gzopen(path.c_str(),"rb");
		if(!file){error=ErrnoText("Cannot open tarball");return false;}
		set<string> names;
		bool ok=true;
		for(;;)
		{
			Header header;
			int got=gzread(file,&header,kBlock);
			if(got==0) break;
			if(got!=(int)kBlock){error="Truncated tarball header";ok=false;break;}
			bool empty=true;
			for(size_t i=0;i<kBlock;++i) if(((const char*)&header)[i]){empty=false;break;}
			if(empty) break;
			if(memcmp(header.magic,"ustar",5)!=0){error="Not a ustar tarball";ok=false;break;}
			if(Checksum(header)!=FromOctal(header.chksum,8)){error="Damaged tarball header";ok=false;break;}
			string name(header.prefix,strnlen(header.prefix,sizeof(header.prefix)));
			if(!name.empty()) name+="/";
			name+=string(header.name,strnlen(header.name,sizeof(header.name)));
			unsigned long long size=FromOctal(header.size,12);
			char type=header.typeflag;
			bool directory=type=='5';
			if(directory && name[name.size()-1]!='/') name+="/";
			if(!SafeArchivePath(name)||!names.insert(name).second)
			{error="Unsupported or unsafe entry in tarball: "+name;ok=false;break;}
			unsigned long long padded=(size+kBlock-1)/kBlock*kBlock;
			if(type!='0'&&type!='\0'&&!directory)
			{
				// Links and specials are skipped, body and all.
				if(gzseek(file,(z_off_t)padded,SEEK_CUR)<0){error="Damaged tarball";ok=false;break;}
				continue;
			}
			if(directory)
			{
				if(!folder.MakeDirectory(name.substr(0,name.size()-1),error)){ok=false;break;}
				continue;
			}
			FILE *output=folder.OpenWrite(name,error);
			if(!output){ok=false;break;}
			unsigned char buffer[65536];unsigned long long remaining=size;
			while(remaining)
			{
				unsigned int n=(unsigned int)(remaining<sizeof(buffer)?remaining:sizeof(buffer));
				if(gzread(file,buffer,n)!=(int)n||fwrite(buffer,1,n,output)!=n){ok=false;break;}
				remaining-=n;
			}
			string closeError;
			if(!folder.CloseWrite(output,closeError)) ok=false;
			if(!ok){folder.RemoveEntry(name);error="Damaged data for "+name;break;}
			unsigned long long slack=padded-size;
			if(slack && gzseek(file,(z_off_t)slack,SEEK_CUR)<0){error="Damaged tarball";ok=false;break;}
		}
		gzclose(file);
		return ok;
	}

	Tar::Writer::Writer():m_File(NULL)
	{
	}

	Tar::Writer::~Writer()
	{
		if(m_File) gzclose((gzFile)m_File);
		if(!m_Path.empty()) unlink(m_Path.c_str());
	}

	// An exclusively created sibling, replaced atomically on Finish.
	bool Tar::Writer::Open(const string &path, string &error)
	{
		string pattern=path+".tmp-XXXXXX";
		vector<char> name(pattern.begin(),pattern.end()); name.push_back(0);
		int fd=mkstemp(&name[0]);
		if(fd<0){error=ErrnoText("Cannot create tarball");return false;}
		m_Path=&name[0];
		// "T" writes transparently (no gzip framing) for a plain .tar.
		m_File=gzdopen(fd,Compressed(path)?"wb6":"wbT");
		if(!m_File){close(fd);error=ErrnoText("Cannot create tarball");return false;}
		return true;
	}

	bool Tar::Writer::Put(const void *data, size_t size, string &error)
	{
		if(size && gzwrite((gzFile)m_File,data,(unsigned int)size)!=(int)size)
		{error="Error writing tarball";return false;}
		return true;
	}

	bool Tar::Writer::Header(const string &name, unsigned long long size, char type, string &error)
	{
		if(!SafeArchivePath(name)){error="Unsafe or overlong archive asset path";return false;}
		if(!m_Names.insert(name).second)
		{error="Two bundled assets map to the same archive path: "+name;return false;}
		if(size>=(1ULL<<33)){error="Asset exceeds the ustar size limit";return false;}
		Spumoni::Header header;
		memset(&header,0,sizeof(header));
		string tail=name,head;
		if(tail.size()>sizeof(header.name))
		{
			// Split at a slash so that prefix/name rejoin to the path.
			size_t cut=tail.rfind('/',sizeof(header.name));
			while(cut!=string::npos && tail.size()-cut-1>sizeof(header.name))
				cut=cut?tail.rfind('/',cut-1):string::npos;
			if(cut==string::npos||cut>sizeof(header.prefix))
			{error="Archive path too long for ustar: "+name;return false;}
			head=tail.substr(0,cut);tail=tail.substr(cut+1);
		}
		memcpy(header.name,tail.data(),tail.size());
		memcpy(header.prefix,head.data(),head.size());
		Octal(header.mode,8,type=='5'?0755:0644);
		Octal(header.uid,8,0);Octal(header.gid,8,0);
		Octal(header.size,12,size);
		Octal(header.mtime,12,(unsigned long long)time(NULL));
		header.typeflag=type;
		memcpy(header.magic,"ustar",6);
		memcpy(header.version,"00",2);
		Octal(header.chksum,7,Checksum(header));header.chksum[7]=' ';
		return Put(&header,kBlock,error);
	}

	bool Tar::Writer::Body(const unsigned char *memory, size_t memorySize, FILE *input,
		unsigned long long size, string &error)
	{
		unsigned char buffer[65536];unsigned long long total=0;
		if(input)
		{
			for(;;)
			{
				size_t n=fread(buffer,1,sizeof(buffer),input);
				if(n && !Put(buffer,n,error)) return false;
				total+=n;
				if(n<sizeof(buffer)){if(ferror(input)){error="Error reading asset";return false;}break;}
			}
			if(total!=size){error="Asset changed size while being archived";return false;}
		}
		else
		{
			if(!Put(memory,memorySize,error)) return false;
			total=memorySize;
		}
		size_t slack=(size_t)((kBlock-total%kBlock)%kBlock);
		if(slack){memset(buffer,0,slack);return Put(buffer,slack,error);}
		return true;
	}

	bool Tar::Writer::AddMemory(const string &name, const string &data, string &error)
	{
		return Header(name,data.size(),'0',error)&&
			Body((const unsigned char*)data.data(),data.size(),NULL,data.size(),error);
	}

	bool Tar::Writer::AddFile(const string &name, const string &path, string &error)
	{
		struct stat st;
		if(stat(path.c_str(),&st)!=0||!S_ISREG(st.st_mode)){error=ErrnoText("Cannot read asset "+path);return false;}
		FILE *input=fopen(path.c_str(),"rb");
		if(!input){error=ErrnoText("Cannot read asset "+path);return false;}
		bool ok=Header(name,(unsigned long long)st.st_size,'0',error)&&
			Body(NULL,0,input,(unsigned long long)st.st_size,error);
		fclose(input);
		return ok;
	}

	bool Tar::Writer::AddDirectory(const string &name, string &error)
	{
		string directory=name[name.size()-1]=='/' ? name : name+"/";
		return Header(directory,0,'5',error);
	}

	bool Tar::Writer::Finish(const string &path, string &error)
	{
		unsigned char zeros[kBlock*2];memset(zeros,0,sizeof(zeros));
		if(!Put(zeros,sizeof(zeros),error)) return false;
		int closed=gzclose((gzFile)m_File);
		m_File=NULL;
		if(closed!=Z_OK){error="Error closing tarball";return false;}
		if(rename(m_Path.c_str(),path.c_str())!=0)
		{error=ErrnoText("Cannot replace tarball");return false;}
		m_Path.clear();
		return true;
	}

} // namespace Spumoni
