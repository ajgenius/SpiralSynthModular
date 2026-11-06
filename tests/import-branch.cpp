// SPDX-License-Identifier: GPL-2.0-or-later
#include "PatchProject.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

using namespace Spiral::File;

static std::string Scratch(const char *name)
{
	const char *dir = getenv("TMPDIR");
	std::string path = dir && *dir ? dir : "/tmp";
	if (path[path.size()-1] != '/') path += '/';
	return path + name;
}

int main()
{
	const std::string package = Scratch("import-branch-test.ssmp");
	const std::string foreign = Scratch("import-branch-foreign.ssm");
	std::remove(package.c_str());

	const std::string mine = "SpiralSynthModular File Ver 4\nmine\n";
	const std::string theirs = "SpiralSynthModular File Ver 4\ntheirs\n";
	{
		std::ofstream out(foreign.c_str());
		out << theirs;
	}

	std::string error;
	{
		Project project("");
		project.Source().Set(mine);
		assert(project.SaveAs(package, error));
		assert(project.GetIdentity().Branches.size() == 1);
	}

	std::string brought;
	{
		std::ifstream in(foreign.c_str());
		std::ostringstream bytes;
		bytes << in.rdbuf();
		brought = bytes.str();
	}

	std::string ours;
	{
		Project project(package);
		assert(project.OpenPackage("", error));
		ours = project.GetIdentity().ActiveBranchID;
		project.Source().Set(brought);
		assert(project.CreateSavePoint("Theirs", false, error, true));
	}

	{
		Project project(package);
		assert(project.OpenPackage("", error));
		const Spumoni::Identity &identity = project.GetIdentity();
		assert(identity.Branches.size() == 2);

		const Spumoni::Branch *imported = NULL, *original = NULL;
		for (size_t i = 0; i < identity.Branches.size(); ++i)
		{
			if (identity.Branches[i].ID == ours)
				original = &identity.Branches[i];
			else
				imported = &identity.Branches[i];
		}
		assert(original && imported);

		assert(imported->Kind == "imported");
		assert(imported->ParentID.empty());
		assert(imported->ForkSaveID.empty());

		assert(project.SwitchBranch(imported->ID, error));
		assert(project.Source().Bytes() == brought);

		assert(project.SwitchBranch(ours, error));
		assert(project.Source().Bytes() == mine);
	}

	std::remove(package.c_str());
	std::remove(foreign.c_str());
	std::puts("An imported branch is independent and keeps its own patch");
	return 0;
}
