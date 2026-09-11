//
//  LADSPAInfo.C - Class for indexing information on LADSPA Plugins
//
//  Copyleft (C) 2002  Mike Rawes <myk@waxfrenzy.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <config.h>

#include <vector>
#include <string>
#include <list>
#include <map>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>

#include <ladspa.h>

#ifdef HAVE_LIBLRDF
#include <lrdf.h>
#endif

#include "LADSPAInfo.h"

using namespace std;

LADSPAInfo *LADSPAInfo::m_Singleton = NULL;

LADSPAInfo *
LADSPAInfo::Get()
{
	if (!m_Singleton)
		m_Singleton = new LADSPAInfo(false, "");
	return m_Singleton;
}

void
LADSPAInfo::PackUpAndGoHome()
{
	if (m_Singleton)
	{
		delete m_Singleton;
		m_Singleton = NULL;
	}
}

LADSPAInfo::LADSPAInfo(bool override,
                       const char *path_list)
{
	m_ExtraPaths = NULL;
	m_MaxInputPortCount = 0;
	if (path_list && strlen(path_list) > 0) {
		m_ExtraPaths = strdup(path_list);
	}
	m_LADSPAPathOverride = override;

	RescanPlugins();
}

LADSPAInfo::~LADSPAInfo()
{
	CleanUp();
	if (m_ExtraPaths) {
		free(m_ExtraPaths);
		m_ExtraPaths = NULL;
	}
}

void
LADSPAInfo::RescanPlugins(void)
{
	CleanUp();

	if (!m_LADSPAPathOverride) {
		char *ladspa_path = getenv("LADSPA_PATH");
		if (ladspa_path) {
			ScanPathList(ladspa_path, &LADSPAInfo::ExaminePluginLibrary);
		} else {
			cerr << "WARNING: LADSPA_PATH environment variable not set" << endl;
			cerr << "         Assuming /usr/lib/ladspa:/usr/local/lib/ladspa" << endl;
			ScanPathList("/usr/lib/ladspa:/usr/local/lib/ladspa", &LADSPAInfo::ExaminePluginLibrary);
		}
	}

	if (m_ExtraPaths) {
		ScanPathList(m_ExtraPaths, &LADSPAInfo::ExaminePluginLibrary);
	}

	if (m_Plugins.size() == 0) {
		cerr << "WARNING: No plugins found" << endl;
	} else {
		cerr << m_Plugins.size() << " plugins found in " << m_Libraries.size() << " libraries" << endl;

#ifdef HAVE_LIBLRDF
		lrdf_init();

		char *rdf_path = getenv("LADSPA_RDF_PATH");

		if (rdf_path) {
			ScanPathList(rdf_path, &LADSPAInfo::ExamineRDFFile);
		} else {
			cerr << "WARNING: LADSPA_RDF_PATH environment variable not set" << endl;
			cerr << "         Assuming /usr/share/ladspa/rdf:/usr/local/share/ladspa/rdf" << endl;
			ScanPathList("/usr/share/ladspa/rdf:/usr/local/share/ladspa/rdf", &LADSPAInfo::ExamineRDFFile);
		}
		MetadataRDFDescend(LADSPA_BASE "Plugin", 0);

		if (m_RDFURIs.empty()) {
			RDFURIInfo ri;
			ri.URI = LADSPA_BASE "Plugin";
			ri.Label = "LADSPA";
			m_RDFURIs.push_back(ri);
			m_RDFURILookup[ri.URI] = 0;
			m_RDFLabelLookup["LADSPA"] = 0;
		}

		list<unsigned long> rdf_p;

		for (vector<RDFURIInfo>::iterator ri = m_RDFURIs.begin(); ri != m_RDFURIs.end(); ri++) {
			rdf_p.insert(rdf_p.begin(), ri->Plugins.begin(), ri->Plugins.end());
		}

		/* unique() only collapses consecutive duplicates — sort first. */
		rdf_p.sort();
		rdf_p.unique();
		/* Fill URI 0 with plugins the RDF tree never mentioned.
		   The old last_p=0 start skipped index 0 when it was unclassified. */
		unsigned long next = 0;
		for (list<unsigned long>::iterator p = rdf_p.begin(); p != rdf_p.end(); p++) {
			while (next < *p && next < m_Plugins.size()) {
				m_RDFURIs[0].Plugins.push_back(next);
				next++;
			}
			if (next == *p)
				next++;
		}
		while (next < m_Plugins.size()) {
			m_RDFURIs[0].Plugins.push_back(next);
			next++;
		}

		lrdf_cleanup();
#else
		RDFURIInfo ri;

		ri.URI = "";
		ri.Label = "LADSPA";

		m_RDFURIs.push_back(ri);
		m_RDFLabelLookup["LADSPA"] = 0;

		for (unsigned long i = 0; i < m_Plugins.size(); i++) {
			m_RDFURIs[0].Plugins.push_back(i);
		}
#endif
	}

	/* Master's LADSPAManager builds the menu once at scan time. */
	DescendGroup("", "LADSPA", 1);
	if (m_SSMMenuList.empty() && !m_Plugins.empty()) {
		BuildUnclassifiedFallback();
	}
}

void
LADSPAInfo::UnloadAllLibraries(void)
{
	for (vector<PluginInfo>::iterator i = m_Plugins.begin();
		i != m_Plugins.end(); i++) {
		if (i->Descriptor) i->Descriptor = NULL;
	}
	for (vector<LibraryInfo>::iterator i = m_Libraries.begin();
		i != m_Libraries.end(); i++) {
		if (i->Handle) {
			dlclose(i->Handle);
			i->Handle = NULL;
		}
		i->RefCount = 0;
	}
}

const LADSPA_Descriptor *
LADSPAInfo::GetDescriptorByID(unsigned long unique_id)
{
	if (m_IDLookup.find(unique_id) == m_IDLookup.end()) {
		cerr << "LADSPA Plugin ID " << unique_id << " not found!" << endl;
		return NULL;
	}

	unsigned long plugin_index = m_IDLookup[unique_id];

	PluginInfo *pi = &(m_Plugins[plugin_index]);
	LibraryInfo *li = &(m_Libraries[pi->LibraryIndex]);

	if (!(pi->Descriptor)) {
		LADSPA_Descriptor_Function desc_func = GetDescriptorFunctionForLibrary(pi->LibraryIndex);
		if (desc_func) pi->Descriptor = desc_func(pi->Index);
	}

	if (pi->Descriptor) {
		li->RefCount++;
	}

	return pi->Descriptor;
}

void
LADSPAInfo::DiscardDescriptorByID(unsigned long unique_id)
{
	if (m_IDLookup.find(unique_id) == m_IDLookup.end()) {
		return;
	}

	unsigned long plugin_index = m_IDLookup[unique_id];

	PluginInfo *pi = &(m_Plugins[plugin_index]);
	LibraryInfo *li = &(m_Libraries[pi->LibraryIndex]);

	pi->Descriptor = NULL;

	if (li->RefCount > 0) {
		li->RefCount--;
		if (li->RefCount == 0 && li->Handle) {
			dlclose(li->Handle);
			li->Handle = NULL;
		}
	}
}

unsigned long
LADSPAInfo::GetIDFromFilenameAndLabel(std::string filename,
                                      std::string label)
{
	bool library_loaded = false;

	if (m_FilenameLookup.find(filename) == m_FilenameLookup.end()) {
		cerr << "LADSPA Library " << filename << " not found!" << endl;
		return 0;
	}

	unsigned long library_index = m_FilenameLookup[filename];

	if (!(m_Libraries[library_index].Handle)) library_loaded = true;

	LADSPA_Descriptor_Function desc_func = GetDescriptorFunctionForLibrary(library_index);

	if (!desc_func) {
		return 0;
	}

	const LADSPA_Descriptor *desc;
	for (unsigned long i = 0; (desc = desc_func(i)) != NULL; i++) {
		string l = desc->Label ? desc->Label : "";
		if (l == label) {
			unsigned long id = desc->UniqueID;
			if (library_loaded && m_Libraries[library_index].Handle) {
				dlclose(m_Libraries[library_index].Handle);
				m_Libraries[library_index].Handle = NULL;
			}
			return id;
		}
	}

	cerr << "Plugin " << label << " not found in library " << filename << endl;
	return 0;
}

unsigned long
LADSPAInfo::GetPluginListEntryByID(unsigned long unique_id)
{
	unsigned long j = 0;
	for (vector<PluginEntry>::iterator i = m_SSMMenuList.begin();
		i != m_SSMMenuList.end(); i++, j++) {
		if (i->UniqueID == unique_id) return j;
	}
	return m_SSMMenuList.size();
}

std::string
LADSPAInfo::GetPluginNameByID(unsigned long unique_id) const
{
	if (m_IDLookup.find(unique_id) == m_IDLookup.end())
		return std::string();
	unsigned long plugin_index = m_IDLookup.find(unique_id)->second;
	return m_Plugins[plugin_index].Name;
}

static string
EscapeMenuName(const string &src)
{
	string name;
	string::size_type x = src.find_first_of("/|");
	if (x == string::npos) {
		return src;
	}
	string::size_type last_x = 0;
	while (x != string::npos) {
		name += src.substr(last_x, x - last_x) + '\\' + src[x];
		last_x = x + 1;
		x = src.find_first_of("/|", last_x);
	}
	name += src.substr(last_x);
	return name;
}

void
LADSPAInfo::BuildUnclassifiedFallback(void)
{
	list<PluginEntry> plugins;
	for (unsigned long i = 0; i < m_Plugins.size(); i++) {
		PluginEntry pe;
		pe.Depth = 2;
		pe.UniqueID = m_Plugins[i].UniqueID;
		pe.Name = string("Unclassified/") + EscapeMenuName(m_Plugins[i].Name);
		plugins.push_back(pe);
	}
	plugins.sort();
	for (list<PluginEntry>::iterator p = plugins.begin(); p != plugins.end(); p++) {
		m_SSMMenuList.push_back(*p);
	}
}

void
LADSPAInfo::DescendGroup(string prefix,
                         const string group,
                         unsigned int depth)
{
	if (depth > 64)
		return;

	list<string> groups = GetSubGroups(group);

	if (prefix.length() > 0) {
		prefix += "/";
	}

	for (list<string>::iterator g = groups.begin(); g != groups.end(); g++) {
		DescendGroup(prefix + EscapeMenuName(*g), *g, depth + 1);
	}
	if (m_RDFLabelLookup.find(group) != m_RDFLabelLookup.end()) {
		unsigned long uri_index = m_RDFLabelLookup[group];
		if (uri_index >= m_RDFURIs.size())
			return;

		if (prefix.length() == 0) {
			prefix = "Unclassified/";
			depth = depth + 1;
		}

		list<PluginEntry> plugins;

		for (vector<unsigned long>::iterator p = m_RDFURIs[uri_index].Plugins.begin();
			p != m_RDFURIs[uri_index].Plugins.end(); p++) {

			if (*p >= m_Plugins.size())
				continue;

			PluginInfo *pi = &(m_Plugins[*p]);

			PluginEntry pe;
			pe.Depth = depth;
			pe.UniqueID = pi->UniqueID;
			pe.Name = prefix + EscapeMenuName(pi->Name);
			plugins.push_back(pe);
		}
		plugins.sort();

		for (list<PluginEntry>::iterator i = plugins.begin();
			i != plugins.end(); ) {
			string name = i->Name;

			i++;
			unsigned long n = 2;
			while ((i != plugins.end()) && (i->Name == name)) {
				stringstream s;
				s << n;
				i->Name = name + " (" + s.str() + ")";
				n++;
				i++;
			}
		}

		for (list<PluginEntry>::iterator p = plugins.begin(); p != plugins.end(); p++) {
			m_SSMMenuList.push_back(*p);
		}
	}
}

list<string>
LADSPAInfo::GetSubGroups(const string group)
{
	list<string> groups;
	unsigned long uri_index;

	if (m_RDFLabelLookup.find(group) == m_RDFLabelLookup.end()) {
		return groups;
	} else {
		uri_index = m_RDFLabelLookup[group];
	}

	if (uri_index >= m_RDFURIs.size())
		return groups;

	for (vector<unsigned long>::iterator sg = m_RDFURIs[uri_index].Children.begin();
		sg != m_RDFURIs[uri_index].Children.end(); sg++) {
		if (*sg < m_RDFURIs.size())
			groups.push_back(m_RDFURIs[*sg].Label);
	}

	groups.sort();

	return groups;
}

void
LADSPAInfo::CleanUp(void)
{
	m_MaxInputPortCount = 0;

	m_IDLookup.clear();
	m_Plugins.clear();

	for (vector<LibraryInfo>::iterator i = m_Libraries.begin();
		i != m_Libraries.end(); i++) {
		if (i->Handle) dlclose(i->Handle);
	}

	m_Libraries.clear();
	m_Paths.clear();

	m_RDFURILookup.clear();
	m_RDFLabelLookup.clear();
	m_RDFURIs.clear();
	m_FilenameLookup.clear();
	m_SSMMenuList.clear();
	/* Keep m_ExtraPaths — RescanPlugins needs it. Freed in the dtor. */
}

void
LADSPAInfo::ScanPathList(const char *path_list,
                         void (LADSPAInfo::*ExamineFunc)(const string,
                                                         const string))
{
	const char *start;
	const char *end;
	int extra;
	char *path;
	string basename;
	DIR *dp;
	struct dirent *ep;
	struct stat sb;

	if (!path_list)
		return;

	start = path_list;
	while (*start != '\0') {
		while (*start == ':') start++;
		end = start;
		while (*end != ':' && *end != '\0') end++;

		if (end - start > 0) {
			extra = (*(end - 1) == '/') ? 0 : 1;
			path = (char *)malloc(end - start + 1 + extra);
			if (path) {
				strncpy(path, start, end - start);
				if (extra == 1) path[end - start] = '/';
				path[end - start + extra] = '\0';

				dp = opendir(path);
				if (!dp) {
					cerr << "WARNING: Could not open path " << path << endl;
				} else {
					while ((ep = readdir(dp))) {
						basename = ep->d_name;
						if (!stat((path + basename).c_str(), &sb)) {
							if (S_ISREG(sb.st_mode)) (*this.*ExamineFunc)(path, basename);
						}
					}
					closedir(dp);
				}
				free(path);
			}
		}
		start = end;
	}
}

void
LADSPAInfo::ExaminePluginLibrary(const string path,
                                 const string basename)
{
	void *handle;
	LADSPA_Descriptor_Function desc_func;
	const LADSPA_Descriptor *desc;
	string fullpath = path + basename;

	handle = dlopen(fullpath.c_str(), RTLD_LAZY);

	if (!handle) {
		cerr << "WARNING: File " << fullpath
			<< " could not be examined" << endl;
		cerr << "dlerror() output:" << endl;
		cerr << dlerror() << endl;
	} else {

		desc_func = (LADSPA_Descriptor_Function)dlsym(handle,
													"ladspa_descriptor");
		if (!desc_func) {
			cerr << "WARNING: DLL " << fullpath
				<< " has no ladspa_descriptor function" << endl;
			cerr << "dlerror() output:" << endl;
			cerr << dlerror() << endl;
		} else {

			bool library_added = false;
			unsigned long i = 0;
			desc = desc_func(i);
			while (desc) {

				if (m_IDLookup.find(desc->UniqueID) != m_IDLookup.end()) {
					unsigned long plugin_index = m_IDLookup[desc->UniqueID];
					unsigned long library_index = m_Plugins[plugin_index].LibraryIndex;
					unsigned long path_index = m_Libraries[library_index].PathIndex;

					cerr << "WARNING: Duplicated Plugin ID ("
						<< desc->UniqueID << ") found:" << endl;

					cerr << "  Plugin " << m_Plugins[plugin_index].Index
						<< " in library: " << m_Paths[path_index]
						<< m_Libraries[library_index].Basename
						<< " [First instance found]" << endl;
					cerr << "  Plugin " << i << " in library: " << fullpath
						<< " [Duplicate not added]" << endl;
				} else {
					if (CheckPlugin(desc)) {

						unsigned long path_index;
						vector<string>::iterator p = find(m_Paths.begin(), m_Paths.end(), path);
						if (p == m_Paths.end()) {
							path_index = m_Paths.size();
							m_Paths.push_back(path);
						} else {
							path_index = p - m_Paths.begin();
						}

						if (!library_added) {
							LibraryInfo li;
							li.PathIndex = path_index;
							li.Basename = basename;
							li.RefCount = 0;
							li.Handle = NULL;
							m_Libraries.push_back(li);
							m_FilenameLookup[basename] = m_Libraries.size() - 1;
							library_added = true;
						}

						PluginInfo pi;
						pi.LibraryIndex = m_Libraries.size() - 1;
						pi.Index = i;
						pi.UniqueID = desc->UniqueID;
						pi.Label = desc->Label ? desc->Label : "";
						pi.Name = desc->Name ? desc->Name : (desc->Label ? desc->Label : "Unnamed");
						pi.Descriptor = NULL;
						m_Plugins.push_back(pi);

						unsigned long in_port_count = 0;
						if (desc->PortDescriptors) {
							for (unsigned long p = 0; p < desc->PortCount; p++) {
								if (LADSPA_IS_PORT_INPUT(desc->PortDescriptors[p])) {
									in_port_count++;
								}
							}
						}
						if (in_port_count > m_MaxInputPortCount) {
							m_MaxInputPortCount = in_port_count;
						}

						m_IDLookup[desc->UniqueID] = m_Plugins.size() - 1;

					} else {
						cerr << "WARNING: Plugin " << desc->UniqueID << " not added" << endl;
					}
				}

				desc = desc_func(++i);
			}
		}
		dlclose(handle);
	}
}

#ifdef HAVE_LIBLRDF
void
LADSPAInfo::ExamineRDFFile(const std::string path,
                           const std::string basename)
{
	string fileuri = "file://" + path + basename;

	if (lrdf_read_file(fileuri.c_str())) {
		cerr << "WARNING: File " << path + basename << " could not be parsed [Ignored]" << endl;
	}
}

void
LADSPAInfo::MetadataRDFDescend(const char * uri,
                               unsigned long parent)
{
	unsigned long this_uri_index;

	if (!uri)
		return;

	if (m_RDFURILookup.find(uri) == m_RDFURILookup.end()) {

		RDFURIInfo ri;

		ri.URI = uri;

		if (ri.URI == LADSPA_BASE "Plugin") {
			ri.Label = "LADSPA";
		} else {
			char * label = lrdf_get_label(uri);
			if (label) {
				ri.Label = label;
			} else {
				ri.Label = "(No label)";
			}
		}

		lrdf_uris * instances = lrdf_get_instances(uri);
		if (instances) {
			for (long j = 0; j < instances->count; j++) {
				unsigned long uid = lrdf_get_uid(instances->items[j]);
				if (m_IDLookup.find(uid) != m_IDLookup.end()) {
					ri.Plugins.push_back(m_IDLookup[uid]);
				}
			}
			lrdf_free_uris(instances);
		}

		m_RDFURIs.push_back(ri);
		this_uri_index = m_RDFURIs.size() - 1;

		m_RDFURILookup[ri.URI] = this_uri_index;
		m_RDFLabelLookup[ri.Label] = this_uri_index;

		if (this_uri_index > 0 && parent < m_RDFURIs.size()) {
			m_RDFURIs[this_uri_index].Parents.push_back(parent);
			m_RDFURIs[parent].Children.push_back(this_uri_index);
		}

		lrdf_uris * uris = lrdf_get_subclasses(uri);

		if (uris) {
			for (long i = 0; i < uris->count; i++) {
				MetadataRDFDescend(uris->items[i], this_uri_index);
			}
			lrdf_free_uris(uris);
		}

	} else {
		/* Already visited. Link this parent but do not walk subclasses
		   again (diamond graphs would explode; cycles would recurse
		   until the stack died — crash on first LADSPA device). */
		this_uri_index = m_RDFURILookup[uri];
		if (this_uri_index > 0 && parent < m_RDFURIs.size()) {
			m_RDFURIs[this_uri_index].Parents.push_back(parent);
			m_RDFURIs[parent].Children.push_back(this_uri_index);
		}
	}
}
#endif

bool
LADSPAInfo::CheckPlugin(const LADSPA_Descriptor *desc)
{
#define test(t, m) { \
	if (!(t)) { \
		cerr << m << endl; \
		return false; \
	} \
}
	test(desc, "WARNING: NULL LADSPA descriptor");
	test(desc->instantiate, "WARNING: Plugin has no instatiate function");
	test(desc->connect_port, "WARNING: Warning: Plugin has no connect_port funciton");
	test(desc->run, "WARNING: Plugin has no run function");
	test(!(desc->run_adding != 0 && desc->set_run_adding_gain == 0),
			"WARNING: Plugin has run_adding but no set_run_adding_gain");
	test(!(desc->run_adding == 0 && desc->set_run_adding_gain != 0),
			"WARNING: Plugin has set_run_adding_gain but no run_adding");
	test(desc->cleanup, "WARNING: Plugin has no cleanup function");
	test(!LADSPA_IS_INPLACE_BROKEN(desc->Properties),
			"WARNING: Plugin cannot use in place processing");
	test(desc->PortCount, "WARNING: Plugin has no ports");
	test(desc->PortDescriptors, "WARNING: Plugin has no port descriptors");
	test(desc->PortNames, "WARNING: Plugin has no port names");
	/* PortRangeHints may be NULL per the LADSPA spec. */

	return true;
}

LADSPA_Descriptor_Function
LADSPAInfo::GetDescriptorFunctionForLibrary(unsigned long library_index)
{
	if (library_index >= m_Libraries.size())
		return NULL;

	LibraryInfo *li = &(m_Libraries[library_index]);

	if (!(li->Handle)) {

		string fullpath = m_Paths[li->PathIndex];
		fullpath.append(li->Basename);

		li->Handle = dlopen(fullpath.c_str(), RTLD_NOW);
		if (!(li->Handle)) {

			cerr << "WARNING: Plugin library " << fullpath << " cannot be loaded" << endl;
			cerr << "Rescan of plugins recommended" << endl;
			cerr << "dlerror() output:" << endl;
			cerr << dlerror() << endl;
			return NULL;
		}
	}

	const LADSPA_Descriptor_Function desc_func = (LADSPA_Descriptor_Function)dlsym(li->Handle,
																				"ladspa_descriptor");
	if (!desc_func) {

		cerr << "WARNING: DLL " << m_Paths[li->PathIndex] << li->Basename
			<< " has no ladspa_descriptor function" << endl;
		cerr << "Rescan of plugins recommended" << endl;
		cerr << "dlerror() output:" << endl;
		cerr << dlerror() << endl;

		dlclose(li->Handle);
		li->Handle = NULL;
		return NULL;
	}

	return desc_func;
}
