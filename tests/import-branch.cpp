// SPDX-License-Identifier: GPL-2.0-or-later
// Importing a patch as a branch is not a save point: a save point descends
// from the branch it was taken from, an import descends from nothing in this
// project. The host wires the menu item to CreateSavePoint's independent
// flag, and this is what that flag has to mean on disk.
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

	// Something recognisable in each, so it is clear which branch holds what.
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

	// What the menu item does once a file has been chosen.
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

	// Two branches now, and the imported one is a root of its own.
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
		assert(imported->ParentID.empty());	// descends from nothing here
		assert(imported->ForkSaveID.empty());

		// And the patch that came in is the one stored under it.
		assert(project.SwitchBranch(imported->ID, error));
		assert(project.Source().Bytes() == brought);

		// The branch that was already here still holds its own patch.
		assert(project.SwitchBranch(ours, error));
		assert(project.Source().Bytes() == mine);
	}

	std::remove(package.c_str());
	std::remove(foreign.c_str());
	std::puts("An imported branch is independent and keeps its own patch");
	return 0;
}
