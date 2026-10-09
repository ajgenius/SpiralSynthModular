// SPDX-License-Identifier: GPL-2.0-or-later
// What a project says about itself survives a save: the drawer writes the
// document through SetDocument, the package carries it, and a reopen reads
// it back. Asking for the licence text puts the full text in the package
// beside the patch and records where; asking for one this build does not
// carry is not a promise it keeps.
#include "PatchProject.h"
#include "License.h"
#include "Folder.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
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
	const std::string package = Scratch("project-document-test.ssmp");
	std::remove(package.c_str());
	std::string error;

	// The LBITF licence is one this build can inline.
	assert(!LicenseFullText("CC-BY-SA-1.0").empty());
	assert(LicenseFullText("CC-BY-SA-4.0").empty());
	bool offered = false;
	for (size_t i = 0; i < LicensePresetCount(); ++i)
		if (std::string(LicensePresetAt(i).Id) == "CC-BY-SA-1.0")
			offered = LicensePresetAt(i).Bundles;
	assert(offered);

	DocumentSection document;
	document.Title = "Looking Backward into the Future";
	document.Description = "From the UNIT-E album.";
	document.CreatedAt = "2003-10-23";
	document.Credits.push_back(Credit());
	document.Credits.back().Name = "Thom Cherryhomes (TSCHAK)";
	document.Credits.back().Role = "Artist";
	document.Rights.Copyright = "(c) 2003 UNIT-E";
	document.Rights.License = "CC-BY-SA-1.0";
	document.Rights.BundleText = true;

	std::string branch;
	{
		Project project("");
		project.Source().Set("SpiralSynthModular File Ver 4\n");
		project.SetDocument(document);

		// Set then read, before any save: the promise is already recorded.
		const DocumentSection back = project.GetDocument();
		assert(back.Title == document.Title);
		assert(back.Rights.BundleText);
		assert(back.Rights.LicenseFile == "licenses/LICENSE.txt");

		assert(project.SaveAs(package, error));
		branch = project.GetIdentity().ActiveBranchID;
	}

	{
		Project project(package);
		assert(project.OpenPackage("", error));
		const DocumentSection back = project.GetDocument();
		assert(back.Title == document.Title);
		assert(back.Description == document.Description);
		assert(back.CreatedAt == document.CreatedAt);
		assert(back.Credits.size() == 1);
		assert(back.Credits[0].Name == "Thom Cherryhomes (TSCHAK)");
		assert(back.Credits[0].Role == "Artist");
		assert(back.Rights.Copyright == document.Rights.Copyright);
		assert(back.Rights.License == "CC-BY-SA-1.0");
		assert(back.Rights.BundleText);

		// The text is in the package where the document says it is,
		// with the copyright line on top.
		std::string text;
		const std::string path = "branches/" + branch + "/" + back.Rights.LicenseFile;
		assert(project.Workspace()->IsFile(path));
		assert(project.Workspace()->Read(path, text, error));
		assert(text.find("Copyright (c) 2003 UNIT-E") == 0);
		assert(text.find("Attribution-ShareAlike 1.0") != std::string::npos);

		// Switching to a licence with no text drops the promise, and
		// the next save writes no file.
		DocumentSection changed = back;
		changed.Rights.License = "CC-BY-SA-4.0";
		project.SetDocument(changed);
		assert(!project.GetDocument().Rights.BundleText);
		assert(project.GetDocument().Rights.LicenseFile.empty());
		assert(project.SaveAs(package, error));
	}

	{
		Project project(package);
		assert(project.OpenPackage("", error));
		const std::string path = "branches/" + project.GetIdentity().ActiveBranchID
			+ "/licenses/LICENSE.txt";
		assert(!project.Workspace()->IsFile(path));
		assert(project.GetDocument().Rights.License == "CC-BY-SA-4.0");

		// Claiming nothing leaves no document at all.
		project.SetDocument(DocumentSection());
		assert(project.GetDocument().Empty());
	}

	std::remove(package.c_str());
	std::puts("A project's document round-trips, and the licence text goes with it when asked");
	return 0;
}
