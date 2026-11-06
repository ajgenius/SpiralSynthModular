// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_TEST_CORPUS_H
#define SPIRAL_PANELS_TEST_CORPUS_H

#include "JSON.h"
#include "PanelDocument.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

static int failures = 0;

#define CHECK(condition, what) \
	do \
	{ \
		if (!(condition)) \
		{ \
			std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, std::string(what).c_str()); \
			++failures; \
		} \
	} \
	while (0)

static std::string Environment(const char *name, const char *fallback)
{
	const char *value = std::getenv(name);

	return value && *value ? value : fallback;
}

static std::string TopSource()
{
	return Environment("TOP_SRCDIR", "../..");
}

static std::string ContractDirectory()
{
	return Environment("SRCDIR", ".") + "/contracts";
}

static bool Exists(const std::string &path)
{
	struct stat info;

	return stat(path.c_str(), &info) == 0;
}

static void AddPanels(const std::string &directory, std::vector<std::string> &paths)
{
	DIR *dir = opendir(directory.c_str());

	if (!dir)
		return;

	while (struct dirent *entry = readdir(dir))
	{
		const std::string info = directory + "/" + entry->d_name + "/info.json";

		if (entry->d_name[0] == '.' || !Exists(info))
			continue;

		Spumoni::JSONOwner root(Spumoni::ParseJSON(info.c_str()));

		if (root.get() && root->Get("ui"))
			paths.push_back(info);
	}

	closedir(dir);
}

static std::vector<std::string> CorpusPanels()
{
	std::vector<std::string> paths;
	AddPanels(TopSource() + "/Plugins/Devices", paths);
	AddPanels(TopSource() + "/libs/libspiralcore/Devices", paths);
	AddPanels(TopSource() + "/Plugins/Panels", paths);
	std::sort(paths.begin(), paths.end());

	return paths;
}

static std::string CorpusPanel(const std::string &path)
{
	Spumoni::JSONOwner root(Spumoni::ParseJSON(path.c_str()));
	const Spumoni::JSON *ui = root.get() ? root->Get("ui") : NULL;

	return ui ? ui->Stringify(true) : std::string();
}

static std::string Canonical(const std::string &text)
{
	Spumoni::JSONOwner value(Spumoni::ParseJSONText(text));

	return value.get() ? value->Stringify(false) : std::string("<not json>");
}

static std::string Saved(const Spiral::Panels::Document &document)
{
	std::string text, error;

	if (!document.Save(text, error))
		return "<save failed: " + error + ">";

	return Canonical(text);
}

static bool LoadText(Spiral::Panels::Document &document, const std::string &text)
{
	std::string error;

	return document.Load(text, error);
}

static std::string ReadFile(const std::string &path)
{
	std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
	std::ostringstream text;
	text << in.rdbuf();

	return text.str();
}

#endif
