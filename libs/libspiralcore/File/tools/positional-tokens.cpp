// SPDX-License-Identifier: GPL-2.0-or-later
// positional-tokens — decode positional .ssm files under the 0.2.x contract
// and print, one JSON object per file, the token stream the reader consumed
// and the index range of each device and its plugin state:
//   {Path, Status, Tokens:{values,between}, Spans:[{ID,PluginID,Record,State}], Diagnostics}
// Concatenating between[0] value[0] between[1] ... reproduces the file.
//
//   positional-tokens <patch.ssm> [more.ssm ...]
//
// SPIRALCORE_SCHEMA_DIR (or the build-time default) locates the contract.
#include "JSON.h"
#include "PositionalReader.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

using spiralcore::Description;
using spiralcore::PositionalReader;

static bool ReadBytes(const char *path, std::string &bytes)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
		return false;
	std::ostringstream buffer;
	buffer << file.rdbuf();
	bytes = buffer.str();
	return true;
}

static std::string Quote(const std::string &text)
{
	std::string out = "\"";
	for (size_t i = 0; i < text.size(); ++i)
	{
		unsigned char c = text[i];
		if (c == '"' || c == '\\')
			out += '\\', out += c;
		else if (c == '\n')
			out += "\\n";
		else if (c == '\r')
			out += "\\r";
		else if (c == '\t')
			out += "\\t";
		else if (c < 0x20)
		{
			char hex[8];
			std::sprintf(hex, "\\u%04x", c);
			out += hex;
		}
		else
			out += c;
	}
	return out + "\"";
}

static void PrintStrings(const std::vector<std::string> &list)
{
	std::printf("[");
	for (size_t i = 0; i < list.size(); ++i)
		std::printf("%s%s", i ? "," : "", Quote(list[i]).c_str());
	std::printf("]");
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		std::fprintf(stderr, "usage: %s <patch.ssm> [more.ssm ...]\n", argv[0]);
		return 2;
	}

	const char *env = std::getenv("SPIRALCORE_SCHEMA_DIR");
	std::string dir = env && *env ? env : SPIRALCORE_SCHEMA_DIR;
	std::string error;
	std::auto_ptr<Spumoni::JSON> contract(Spumoni::ParseJSON((dir + "/SpiralPositionalText-0.2.x.json").c_str(), &error));
	if (!contract.get())
	{
		std::fprintf(stderr, "%s/SpiralPositionalText-0.2.x.json: %s\n", dir.c_str(), error.c_str());
		return 2;
	}
	std::auto_ptr<Spumoni::JSON> history(Spumoni::ParseJSON((dir + "/SpiralPositionalText.history.json").c_str(), &error));
	if (!history.get())
	{
		std::fprintf(stderr, "%s/SpiralPositionalText.history.json: %s\n", dir.c_str(), error.c_str());
		return 2;
	}

	PositionalReader reader(*contract, history.get());
	int failures = 0;
	for (int i = 1; i < argc; ++i)
	{
		std::string bytes;
		Description tokens;
		if (!ReadBytes(argv[i], bytes) || !reader.Read(bytes, tokens, error))
		{
			std::fprintf(stderr, "%s: %s\n", argv[i], error.c_str());
			++failures;
			continue;
		}

		std::printf("{\"Path\":%s,\"Status\":%s,\"Tokens\":{\"values\":", Quote(argv[i]).c_str(), Quote(reader.Status()).c_str());
		PrintStrings(tokens.Values());
		std::printf(",\"between\":");
		PrintStrings(tokens.Between());
		std::printf("},\"Spans\":[");
		const std::vector<PositionalReader::Device> &devices = reader.Devices();
		for (size_t d = 0; d < devices.size(); ++d)
			std::printf("%s{\"ID\":%ld,\"PluginID\":%ld,\"Record\":[%lu,%lu],\"State\":[%lu,%lu]}", d ? "," : "",
			            devices[d].id, devices[d].pluginID,
			            (unsigned long)devices[d].record.begin, (unsigned long)devices[d].record.end,
			            (unsigned long)devices[d].state.begin, (unsigned long)devices[d].state.end);
		std::printf("],\"Diagnostics\":[");
		const std::vector<PositionalReader::Diagnostic> &notes = reader.Diagnostics();
		for (size_t n = 0; n < notes.size(); ++n)
		{
			std::printf("%s{\"Code\":%s,\"Field\":%s,\"Offset\":%lu,\"Message\":%s", n ? "," : "",
			            Quote(notes[n].code).c_str(), Quote(notes[n].field).c_str(),
			            (unsigned long)notes[n].offset, Quote(notes[n].message).c_str());
			if (notes[n].index >= 0)
				std::printf(",\"Index\":%ld", notes[n].index);
			std::printf("}");
		}
		std::printf("]}\n");
	}
	return failures ? 1 : 0;
}
