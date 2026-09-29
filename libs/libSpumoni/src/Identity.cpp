// SPDX-License-Identifier: GPL-2.0-or-later
// Spumoni::Identity — see Identity.h. The bookkeeping and the UUID helpers
// came out of Spumoni::File::Archive (ChatGPT / GROK originals) by way of
// libSSMCore's ProjectIdentity.cpp.
#include "Identity.h"

#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>

using namespace Spumoni;

namespace
{
	bool IsHex(char c)
	{
		return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');
	}
}

bool Spumoni::IsUUID(const std::string &value)
{
	if(value.size()!=36) return false;
	for(size_t i=0;i<value.size();++i)
	{
		if(i==8||i==13||i==18||i==23)
		{
			if(value[i]!='-') return false;
		}
		else if(!IsHex(value[i])) return false;
	}
	return true;
}

std::string Spumoni::GenerateUUID()
{
	unsigned char bytes[16];memset(bytes,0,sizeof(bytes));
	FILE *random=fopen("/dev/urandom","rb");
	bool haveRandom=random&&fread(bytes,1,sizeof(bytes),random)==sizeof(bytes);
	if(random) fclose(random);
	if(!haveRandom)
	{
		static uint64_t sequence=0;
		struct timeval now;gettimeofday(&now,NULL);
		uint64_t state=((uint64_t)now.tv_sec<<32)^(uint64_t)now.tv_usec^
			((uint64_t)getpid()<<16)^(++sequence);
		for(size_t i=0;i<sizeof(bytes);++i)
		{
			state^=state<<13;state^=state>>7;state^=state<<17;
			bytes[i]=(unsigned char)(state>>((i&7)*8));
		}
	}
	bytes[6]=(unsigned char)((bytes[6]&0x0f)|0x40);
	bytes[8]=(unsigned char)((bytes[8]&0x3f)|0x80);
	char text[37];
	snprintf(text,sizeof(text),
		"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		bytes[0],bytes[1],bytes[2],bytes[3],bytes[4],bytes[5],bytes[6],bytes[7],
		bytes[8],bytes[9],bytes[10],bytes[11],bytes[12],bytes[13],bytes[14],bytes[15]);
	return text;
}

std::string Identity::TrimName(const std::string &name)
{
	std::string trimmed=name;
	while(!trimmed.empty() && (trimmed[0]==' '||trimmed[0]=='\t'))
		trimmed.erase(0,1);
	while(!trimmed.empty() && (trimmed[trimmed.size()-1]==' '||
		trimmed[trimmed.size()-1]=='\t'))
		trimmed.erase(trimmed.size()-1);
	return trimmed;
}

Branch *Identity::Find(const std::string &id)
{
	for(size_t i=0;i<Branches.size();++i)
		if(Branches[i].ID==id) return &Branches[i];
	return NULL;
}

Branch *Identity::FindByName(const std::string &name)
{
	for(size_t i=0;i<Branches.size();++i)
		if(Branches[i].Name==name) return &Branches[i];
	return NULL;
}

bool Identity::NameTaken(const std::string &name, const std::string &exceptID) const
{
	for(size_t i=0;i<Branches.size();++i)
		if(Branches[i].ID!=exceptID && Branches[i].Name==name)
			return true;
	return false;
}

void Identity::EnsureActiveListed()
{
	if(ActiveBranchID.empty()) return;
	if(Branch *existing=Find(ActiveBranchID))
	{
		if(!ActiveBranchName.empty())
			existing->Name=ActiveBranchName;
		if(existing->Kind.empty()) existing->Kind="named";
		return;
	}
	Branch branch;
	branch.ID=ActiveBranchID;
	branch.Name=ActiveBranchName;
	branch.Kind="named";
	Branches.push_back(branch);
}

void Identity::AdoptUnlisted(const std::string &id)
{
	if(!IsUUID(id) || Find(id)) return;
	Branch branch;
	branch.ID=id;
	branch.Kind="named";
	Branches.push_back(branch);
}

bool Identity::ActivateNamed(const std::string &name, std::string &error)
{
	std::string trimmed=TrimName(name);
	if(trimmed.empty())
	{error="Replace save point name is empty";return false;}
	Branch *found=FindByName(trimmed);
	if(!found)
	{error="No save point named: "+trimmed;return false;}
	ActiveBranchID=found->ID;
	ActiveBranchName=found->Name;
	return true;
}

bool Identity::Fork(const std::string &name, std::string &error)
{
	std::string trimmed=TrimName(name);
	if(trimmed.empty())
	{error="New branch name is empty";return false;}
	if(ActiveBranchID.empty())
	{
		ActiveBranchName=trimmed;
		return true;
	}
	EnsureActiveListed();
	if(NameTaken(trimmed))
	{error="Branch name already exists: "+trimmed;return false;}
	Branch branch;
	branch.ID=GenerateUUID();
	branch.Name=trimmed;
	branch.Kind="named";
	branch.ParentID=ActiveBranchID;
	Branches.push_back(branch);
	ActiveBranchID=branch.ID;
	ActiveBranchName=trimmed;
	return true;
}

std::string Identity::SuggestedName() const
{
	std::string base=ActiveBranchName.empty()?"Untitled":ActiveBranchName;
	for(int n=2;n<1000;++n)
	{
		char text[16];
		snprintf(text,sizeof(text)," %d",n);
		std::string candidate=base+text;
		if(!NameTaken(candidate)) return candidate;
	}
	return base+" "+GenerateUUID();
}

void Identity::Begin()
{
	PackageID=GenerateUUID();
	ActiveBranchID=GenerateUUID();
}
