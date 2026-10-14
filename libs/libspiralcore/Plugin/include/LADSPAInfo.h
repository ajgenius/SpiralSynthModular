//
//  LADSPAInfo.h - Header file for LADSPA Plugin info class
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

#ifndef __ladspa_info_h__
#define __ladspa_info_h__

#include <config.h>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <ladspa.h>

/* UA counterpart of master's Foundation/libSSMPlugins/LADSPAManager.
 * Same scan/index idea (singleton, cached menu list) but kept next to
 * the FLTK plugin — UA has no libSSMPlugins / GTK Device split. */

class LADSPAInfo
{
public:
	static LADSPAInfo *Get();
	static void        PackUpAndGoHome();

	/* Extra path list is only used by Get()'s default scanner if you
	 * construct a one-off instance. The plugin uses Get(). */
	LADSPAInfo(bool override = false, const char *path_list = "");
	~LADSPAInfo();

	void                            RescanPlugins(void);
	void                            UnloadAllLibraries(void);
	const LADSPA_Descriptor        *GetDescriptorByID(unsigned long unique_id);
	void                            DiscardDescriptorByID(unsigned long unique_id);

	unsigned long                   GetIDFromFilenameAndLabel(std::string filename,
	                                                          std::string label);

	struct PluginEntry
	{
		unsigned int    Depth;
		unsigned long   UniqueID;
		std::string     Name;

		bool operator<(const PluginEntry& pe) const
		{
			return (Name<pe.Name);
		}
	};

	/* Cached list, rebuilt on RescanPlugins. Const-ref like master's PluginList(). */
	const std::vector<PluginEntry> &GetMenuList(void) const { return m_SSMMenuList; }

	unsigned long                   GetPluginListEntryByID(unsigned long unique_id);

	unsigned long                   GetMaxInputPortCount(void) const { return m_MaxInputPortCount; }

	bool WasPluginFound(unsigned long unique_id) const
	{
		return (m_IDLookup.find(unique_id) != m_IDLookup.end());
	}

	std::string GetPluginNameByID(unsigned long unique_id) const;

private:
	static LADSPAInfo              *m_Singleton;

	void                            DescendGroup(std::string prefix,
	                                             const std::string group,
	                                             unsigned int depth);
	std::list<std::string>          GetSubGroups(const std::string group);

	void                            CleanUp(void);
	void                            ScanPathList(const char *path_list,
	                                             void (LADSPAInfo::*ExamineFunc)(const std::string,
	                                                                             const std::string));
	void                            ExaminePluginLibrary(const std::string path,
	                                                     const std::string basename);

	bool                            CheckPlugin(const LADSPA_Descriptor *desc);
	LADSPA_Descriptor_Function      GetDescriptorFunctionForLibrary(unsigned long library_index);
	void                            BuildUnclassifiedFallback(void);
#ifdef HAVE_LIBLRDF
	void                            ExamineRDFFile(const std::string path,
	                                               const std::string basename);
	void                            MetadataRDFDescend(const char *uri,
	                                                   unsigned long parent);
#endif

	struct LibraryInfo
	{
		unsigned long               PathIndex;
		std::string                 Basename;
		unsigned long               RefCount;
		void                       *Handle;
	};

	struct PluginInfo
	{
		unsigned long               LibraryIndex;
		unsigned long               Index;
		unsigned long               UniqueID;
		std::string                 Label;
		std::string                 Name;
		const LADSPA_Descriptor    *Descriptor;
	};

	struct RDFURIInfo
	{
		std::string                 URI;
		std::string                 Label;
		std::vector<unsigned long>  Parents;
		std::vector<unsigned long>  Children;
		std::vector<unsigned long>  Plugins;
	};

	typedef std::map<unsigned long,
	                 unsigned long,
	                 std::less<unsigned long> >  IDMap;

	typedef std::map<std::string,
	                 unsigned long,
	                 std::less<std::string> >    StringMap;

	bool                            m_LADSPAPathOverride;
	char                           *m_ExtraPaths;

	std::vector<std::string>        m_Paths;
	std::vector<LibraryInfo>        m_Libraries;
	std::vector<PluginInfo>         m_Plugins;

	IDMap                           m_IDLookup;

	std::vector<RDFURIInfo>         m_RDFURIs;
	StringMap                       m_RDFURILookup;
	StringMap                       m_RDFLabelLookup;

	std::vector<PluginEntry>        m_SSMMenuList;
	StringMap                       m_FilenameLookup;
	unsigned long                   m_MaxInputPortCount;
};

#endif // __ladspa_info_h__
