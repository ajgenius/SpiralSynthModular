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
	assert(writer.SaveAs(path, error));
	assert(writer.CreateSavePoint("Second", false, error));
	Spiral::File::Project reader(path);
	assert(reader.OpenPackage("", error));
	assert(reader.Source().Bytes() == bytes);
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
		assert(unlink(path.c_str()) == 0);

	assert(rmdir(temporary) == 0);
	std::puts("package-project OK");
	return 0;
}
