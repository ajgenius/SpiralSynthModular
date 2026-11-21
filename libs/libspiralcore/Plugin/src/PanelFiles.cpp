// SPDX-License-Identifier: GPL-2.0-or-later
#include <config.h>
#include "PanelFiles.h"
#include "JSON.h"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <sstream>
#include <unistd.h>

using namespace std;
using Spumoni::JSON;

namespace Spiral
{
namespace Core
{

vector<string> PanelFiles::Folders(const string &installed)
{
	vector<string> folders;

	if (const char *path = getenv("SSM_PANEL_PATH"))
	{
		istringstream list(path);
		string folder;

		while (getline(list, folder, ':'))
			if (!folder.empty())
				folders.push_back(folder);
	}

	if (const char *home = getenv("HOME"))
		if (*home)
			folders.push_back(string(home) + "/.spiral/panels");

	if (!installed.empty())
		folders.push_back(installed);

	return folders;
}

static string Key(const char *kind, long id)
{
	ostringstream key;
	key << kind << ':' << id;

	return key.str();
}

PanelFiles::PanelFiles(const vector<string> &folders)
{
	for (size_t f = 0; f < folders.size(); ++f)
	{
		DIR *dir = opendir(folders[f].c_str());

		if (!dir)
			continue;

		// In a folder, by name: which of two files for the same thing is
		// taken does not then depend on the order the folder is read in.
		vector<string> names;

		while (struct dirent *entry = readdir(dir))
			if (entry->d_name[0] != '.')
				names.push_back(entry->d_name);

		closedir(dir);
		sort(names.begin(), names.end());

		for (size_t n = 0; n < names.size(); ++n)
		{
			const string path = folders[f] + "/" + names[n] + "/info.json";

			if (access(path.c_str(), R_OK) != 0)
				continue;

			Spumoni::JSONOwner info(Spumoni::ParseJSON(path.c_str()));
			const JSON *target = info.get() && info->GetType() == JSON::Object ? info->Get("for") : NULL;
			const JSON *ui = target ? info->Get("ui") : NULL;

			if (!target || target->GetType() != JSON::Object || !ui || ui->GetType() != JSON::Object)
				continue;

			static const char *const kinds[] = {"device", "ladspa"};

			for (size_t k = 0; k < sizeof(kinds) / sizeof(kinds[0]); ++k)
			{
				const JSON *what = target->Get(kinds[k]);
				long id = 0;

				if (what && what->Integer(id) && !m_Paths.count(Key(kinds[k], id)))
					m_Paths[Key(kinds[k], id)] = path;
			}
		}
	}
}

string PanelFiles::Path(const char *kind, long id) const
{
	const map<string, string>::const_iterator found = m_Paths.find(Key(kind, id));

	return found == m_Paths.end() ? string() : found->second;
}

JSON *PanelFiles::Read(const char *kind, long id, string *path) const
{
	const string file = Path(kind, id);

	if (file.empty())
		return NULL;

	Spumoni::JSONOwner info(Spumoni::ParseJSON(file.c_str()));
	const JSON *ui = info.get() && info->GetType() == JSON::Object ? info->Get("ui") : NULL;

	if (!ui || ui->GetType() != JSON::Object)
		return NULL;

	if (path)
		*path = file;

	return ui->Duplicate();
}

}
}
