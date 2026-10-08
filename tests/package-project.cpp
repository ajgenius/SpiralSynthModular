// SPDX-License-Identifier: GPL-2.0-or-later
#include "PatchProject.h"
#include "JSON.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <unistd.h>

int main(int argc, char **argv)
{
	assert(argc >= 1 && argc <= 3);
	std::ifstream input(argc > 1 ? argv[1] : PUBLIC_TEST_SOURCE, std::ios::binary);
	assert(input);
	const std::string bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
	char temporary[] = "/tmp/public-project-XXXXXX";
	assert(mkdtemp(temporary));
	const std::string path = argc == 3 ? argv[2] : std::string(temporary) + "/public.ssmp";
	Spiral::File::Project writer("");
	writer.Source().Set(bytes);
	std::string error;
	assert(writer.BeginSidecars(error));
	assert(Spumoni::Path::WriteFile(writer.SidecarDirectory() + "PoshSampler3_0.wav", "first sample", error));
	assert(Spumoni::Path::WriteFile(writer.SidecarDirectory() + "obsolete.wav", "old", error));
	assert(writer.SaveAs(path, error));
	const std::string first = writer.GetIdentity().ActiveBranchID;
	assert(writer.BeginSidecars(error));
	assert(Spumoni::Path::WriteFile(writer.SidecarDirectory() + "PoshSampler3_0.wav", "second sample", error));
	assert(writer.CreateSavePoint("Second", false, error));
	// Relocation must not depend on a directory beside the original package.
	const std::string relocated = argc == 3 ? path : std::string(temporary) + "/moved.ssmp";
	if (argc != 3)
		assert(rename(path.c_str(), relocated.c_str()) == 0);

	Spiral::File::Project reader(relocated);
	assert(reader.OpenPackage("", error));
	assert(reader.Source().Bytes() == bytes);
	std::string sample;
	assert(Spumoni::Path::ReadFile(reader.SidecarDirectory() + "PoshSampler3_0.wav", sample, error));
	assert(sample == "second sample");
	assert(!Spumoni::Path::IsFile(reader.SidecarDirectory() + "obsolete.wav"));
	assert(reader.SwitchBranch(first, error));
	assert(Spumoni::Path::ReadFile(reader.SidecarDirectory() + "PoshSampler3_0.wav", sample, error));
	assert(sample == "first sample");
	const std::string kept = reader.SidecarDirectory();
	assert(!reader.SwitchBranch("missing", error));
	assert(reader.SidecarDirectory() == kept);
	// Saving an empty replacement removes the old samples in that branch.
	if (argc != 3)
	{
		assert(reader.BeginSidecars(error));
		assert(reader.CreateSavePoint(reader.GetIdentity().ActiveBranchName, true, error));
		assert(reader.SwitchBranch(first, error));
		assert(!Spumoni::Path::IsFile(reader.SidecarDirectory() + "PoshSampler3_0.wav"));
	}

	assert(reader.GetIdentity().Branches.size() == 2);
	std::string metadata;
	assert(reader.Workspace()->Read("project.spiral.json", metadata, error));
	std::auto_ptr<Spumoni::JSON> root(Spumoni::ParseJSONText(metadata.c_str(), &error));
	assert(root.get());
	assert(root->Get("format")->Text() == "SpiralSynthModular File Ver 9");
	assert(root->Get("main")->Get("name")->Text() == "patch.spiral.legacy.ssm");
	assert(root->Get("main")->Get("format")->Text() == "SpiralSynthModular File Ver 4");
	Spumoni::Identity identity;
	const Spumoni::Package::Application &app = *reader.GetFormat().Application;
	assert(app.Accept(*root, identity, error));
	root->SetOwned("format", Spumoni::JSON::MakeString("SpiralSynthModular File Ver 10"));
	assert(!app.Accept(*root, identity, error));
	root->SetOwned("format", Spumoni::JSON::MakeString("SpiralSynthModular File Ver 9"));
	root->Get("main")->SetOwned("name", Spumoni::JSON::MakeString("patch.spiral.json"));
	assert(!app.Accept(*root, identity, error));
	if (argc != 3)
		assert(unlink(relocated.c_str()) == 0);

	assert(rmdir(temporary) == 0);
	std::puts("package-project OK");
	return 0;
}
