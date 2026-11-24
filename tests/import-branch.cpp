// SPDX-License-Identifier: GPL-2.0-or-later
#include "PatchProject.h"
#include "PatchFile.h"
#include "Folder.h"
#include "JSON.h"
#include <cassert>
#include <cstdio>

using namespace Spiral::File;

static std::string ReadAsset(const Project &project)
{
	std::string data, error;
	assert(Spumoni::Path::ReadFile(project.SidecarDirectory() + "take.wav", data, error));
	return data;
}

static void Stage(Project &project, const std::string &take, const std::string &title)
{
	std::string error, json;
	Spiral::Core::PatchDescription definition;
	assert(PatchFile::Text(definition, json, error));
	project.Patch().Set(json);
	project.Source().Set("source " + take);
	assert(project.BeginSidecars(error));
	assert(Spumoni::Path::WriteFile(project.SidecarDirectory() + "take.wav", take, error));
	DocumentSection metadata;
	metadata.Title = title;
	project.SetDocument(metadata);
}

int main()
{
	Spumoni::DiskFolder scratch;
	std::string error;
	assert(scratch.Create("ssm-import-branch-", error));
	const std::string package = scratch.Root() + "/mine.ssmp";
	const std::string foreign = scratch.Root() + "/theirs.ssmp";
	const std::string legacy = scratch.Root() + "/legacy.ssm";
	const std::string positional = "SpiralSynthModular File Ver 4\n0\n";
	assert(Spumoni::Path::WriteFile(legacy, positional, error));

	Project target(package), source(foreign);
	Stage(target, "mine", "Album");
	Stage(source, "theirs", "Guest take");
	assert(target.SaveAs(package, error));
	Spumoni::JSONOwner newer(Spumoni::ParseJSONText(source.Patch().Bytes()));
	newer->SetOwned("format", Spumoni::JSON::MakeString("SpiralSynthModular Patch File Ver 5"));
	newer->SetOwned("version", Spumoni::JSON::MakeNumber("5"));
	source.Patch().Set(newer->Stringify(true));
	assert(source.SaveAs(foreign, error));
	const std::string original = target.GetIdentity().ActiveBranchID;
	assert(target.ReadBranchContents(foreign, error));
	assert(target.CreateSavePoint("Guest", false, error, true));
	const std::string guest = target.GetIdentity().ActiveBranchID;
	std::string metadata;
	assert(target.Workspace()->Read("project.spiral.json", metadata, error));
	Spumoni::JSONOwner manifest(Spumoni::ParseJSONText(metadata));
	assert(manifest->Get("main")->Get("format")->Text() == "SpiralSynthModular Patch File Ver 5");
	assert(target.ReadBranchContents(legacy, error));
	assert(target.CreateSavePoint("Legacy", false, error, true));
	const std::string raw = target.GetIdentity().ActiveBranchID;
	assert(target.GetIdentity().Branches.size() == 3);

	Project opened(package);
	assert(opened.OpenPackage(raw, error));
	assert(!opened.HasPatch() && opened.Source().Bytes() == positional);
	assert(opened.Workspace()->Read("project.spiral.json", metadata, error));
	Spumoni::JSONOwner rawManifest(Spumoni::ParseJSONText(metadata));
	assert(rawManifest->Get("main")->Get("name")->Text() == "patch.spiral.legacy.ssm");
	assert(!Spumoni::Path::IsFile(opened.SidecarDirectory() + "take.wav"));

	assert(opened.SwitchBranch(guest, error));
	assert(opened.Source().Bytes() == "source theirs" && ReadAsset(opened) == "theirs");
	Spiral::Core::PatchDescription definition;
	assert(PatchFile::ReadText(opened.Patch().Bytes(), definition, error));
	assert(definition.Document.Title == "Guest take" && !opened.Schema().Empty());
	assert(opened.GetDocument().Title == "Album");
	const Spumoni::Identity &identity = opened.GetIdentity();
	for (size_t i = 0; i < identity.Branches.size(); ++i)
		if (identity.Branches[i].ID == guest)
			assert(identity.Branches[i].Kind == "imported" && identity.Branches[i].ParentID.empty()
				&& identity.Branches[i].ForkSaveID.empty());

	assert(opened.SwitchBranch(original, error));
	assert(opened.Source().Bytes() == "source mine" && ReadAsset(opened) == "mine");
	assert(!opened.ReadBranchContents(scratch.Root() + "/absent.ssm", error));
	assert(opened.HasPatch() && opened.Source().Bytes() == "source mine" && ReadAsset(opened) == "mine");
	std::puts("Independent branches keep their own patch, source, schema, credits and sidecars");
	return 0;
}
