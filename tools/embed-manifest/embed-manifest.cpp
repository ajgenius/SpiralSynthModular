// SPDX-License-Identifier: GPL-2.0-or-later
#include <config.h>
#include "Gzip.h"
#include "JSON.h"
#include "PanelDocument.h"
#include "PanelNode.h"

#include <cstdio>
#include <dlfcn.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

static int Fail(const std::string &path, const std::string &why)
{
	std::cerr << "embed-manifest: " << path << ": " << why << std::endl;

	return 1;
}

static bool Text(const Spumoni::JSON &root, const char *key)
{
	const Spumoni::JSON *value = root.Get(key);

	return value && value->GetType() == Spumoni::JSON::String && !value->Text().empty();
}

static int Dump(const std::string &path)
{
	void *module = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);

	if (!module)
		return Fail(path, dlerror());

	typedef const unsigned char *(*ManifestFunction)(size_t *);
	ManifestFunction manifest = (ManifestFunction)dlsym(module, "SpiralPlugin_GetManifest");

	if (!manifest)
		return Fail(path, "no SpiralPlugin_GetManifest");

	size_t size = 0;
	const unsigned char *data = manifest(&size);
	std::string text, error;

	if (!Spumoni::Gzip::Decompress(data, size, text, error))
		return Fail(path, error);

	Spumoni::JSONOwner root(Spumoni::ParseJSONText(text, &error));

	if (!root.get())
		return Fail(path, error);

	std::cout << root->Stringify(true) << std::endl;

	return 0;
}

static int Embed(const std::string &input, const Spumoni::JSON &root, const std::string &output,
		const std::string &function)
{
	std::string packed, error;

	if (!Spumoni::Gzip::Compress(root.Stringify(false), packed, error))
		return Fail(input, error);

	std::ostringstream out;
	out << "#include <cstddef>\n\nstatic const unsigned char Data[] =\n{";

	for (size_t i = 0; i < packed.size(); ++i)
	{
		out << (i % 16 == 0 ? "\n\t" : " ");
		char byte[8];
		std::snprintf(byte, sizeof(byte), "0x%02x,", (unsigned char)packed[i]);
		out << byte;
	}

	out << "\n};\n\n";

	if (function.empty())
		out << "extern \"C\" const unsigned char *SpiralPlugin_GetManifest(size_t *size)\n";
	else
		out << "const unsigned char *" << function << "(size_t *size)\n";

	out << "{\n\t*size = sizeof(Data);\n\n\treturn Data;\n}\n";
	const std::string temporary = output + ".tmp";
	std::ofstream file(temporary.c_str(), std::ios::binary);
	file << out.str();
	file.close();

	if (!file || std::rename(temporary.c_str(), output.c_str()) != 0)
		return Fail(output, "cannot write");

	return 0;
}

int main(int argc, char **argv)
{
	if (argc == 3 && std::strcmp(argv[1], "--dump") == 0)
		return Dump(argv[2]);

	if (argc == 5 && std::strcmp(argv[1], "--data") == 0)
	{
		std::string error;
		Spumoni::JSONOwner data(Spumoni::ParseJSON(argv[2], &error));

		if (!data.get())
			return Fail(argv[2], error);

		return Embed(argv[2], *data, argv[3], argv[4]);
	}

	if (argc != 3 && argc != 4)
	{
		std::cerr << "usage: embed-manifest info.json output.cpp [function]" << std::endl;
		std::cerr << "       embed-manifest --data file.json output.cpp function" << std::endl;
		std::cerr << "       embed-manifest --dump Module.so" << std::endl;

		return 2;
	}

	const std::string input = argv[1];
	const std::string output = argv[2];
	const std::string function = argc == 4 ? argv[3] : "";
	std::string error;
	Spumoni::JSONOwner root(Spumoni::ParseJSON(input.c_str(), &error));

	if (!root.get())
		return Fail(input, error);

	if (root->GetType() != Spumoni::JSON::Object)
		return Fail(input, "not an object");

	long id = -1;
	const Spumoni::JSON *identifier = root->Get("id");

	if (!identifier || !identifier->Integer(id) || id < 0)
		return Fail(input, "id is not a non-negative integer");

	if (!Text(*root, "name") || !Text(*root, "category") || !Text(*root, "registration"))
		return Fail(input, "name, category and registration are required");

	// A built device should report the host it was actually compiled for.
	Spumoni::JSON *host = root->Get("host");
	if (host && host->GetType() == Spumoni::JSON::Object)
	{
		host->SetOwned("abi", Spumoni::JSON::MakeString(SSM_HOST_ABI));
		host->SetOwned("version", Spumoni::JSON::MakeString(PACKAGE_VERSION));
	}

	const Spumoni::JSON *ui = root->Get("ui");

	if (ui)
	{
		Spiral::Panels::Document document;

		if (!document.Load(*ui, error))
			return Fail(input, "ui: " + error);

		if (!document.Panel() || document.Panel()->Children().empty())
			return Fail(input, "ui: the panel has no controls");
	}

	return Embed(input, *root, output, function);
}
