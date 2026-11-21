// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANEL_FILES_H
#define SPIRAL_PANEL_FILES_H

#include <map>
#include <string>
#include <vector>

namespace Spumoni
{
	class JSON;
}

namespace Spiral
{
namespace Core
{

// A panel can be a file in a panels folder: `<folder>/<Name>/info.json`, which
// says what it is for (`"for": {"device": 9}` or `{"ladspa": 1216}`) and
// holds the panel under `ui`. For a device that has a panel of its own built
// in, a file here is shown in its place; for one that has none it is the
// panel.
class PanelFiles
{
public:
	// The folders looked in, first to last: each of SSM_PANEL_PATH, the
	// user's ~/.spiral/panels, then the installed one. The first folder
	// that has a file for something wins.
	static std::vector<std::string> Folders(const std::string &installed);

	explicit PanelFiles(const std::vector<std::string> &folders);

	// The file that has a panel for this, or nothing.
	std::string Path(const char *kind, long id) const;

	// That file's `ui`, the caller's to delete, or NULL.
	Spumoni::JSON *Read(const char *kind, long id, std::string *path = NULL) const;

private:
	std::map<std::string, std::string> m_Paths;
};

}
}

#endif
