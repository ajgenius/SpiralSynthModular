// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Container::Sniff / ForPath — which kind a path is. The one place
// that knows every kind by name.

#include "Container.h"
#include "Directory.h"
#include "Tar.h"
#include "Zip.h"
#include "Folder.h"

#include <cstdio>
#include <cstring>

using namespace std;

namespace Spumoni
{
	namespace
	{
		const Zip &TheZip(){static const Zip zip;return zip;}
		const Tar &TheTar(){static const Tar tar;return tar;}
		const Directory &TheDirectory(){static const Directory directory;return directory;}

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

	const Container *Container::Sniff(const string &path)
	{
		if(Path::IsDirectory(path)) return &TheDirectory();
		FILE *file=fopen(path.c_str(),"rb");
		if(!file) return NULL;
		unsigned char head[512+8];
		size_t n=fread(head,1,sizeof(head),file);
		fclose(file);
		if(n>=4 && head[0]=='P' && head[1]=='K' && head[2]==3 && head[3]==4) return &TheZip();
		if(n>=2 && head[0]==0x1f && head[1]==0x8b) return &TheTar();
		if(n>=257+5 && memcmp(head+257,"ustar",5)==0) return &TheTar();
		return NULL;
	}

	const Container &Container::ForPath(const string &path)
	{
		if(const Container *found=Sniff(path)) return *found;
		if(HasSuffix(path,"/")) return TheDirectory();
		if(HasSuffix(path,".tar")||HasSuffix(path,".tgz")||HasSuffix(path,".tar.gz")) return TheTar();
		return TheZip();
	}

} // namespace Spumoni
