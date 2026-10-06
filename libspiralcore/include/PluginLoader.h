/*  SpiralSound
 *  Copyleft (C) 2002 David Griffiths <dave@pawfal.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef SPIRAL_PLUGIN_LOADER_H
#define SPIRAL_PLUGIN_LOADER_H

#include <string>
#include <vector>

namespace spiralcore
{

/* A plugin kind says what to look for under the plugin root and what to
   do with a module once its entry symbol resolves. The loader knows
   nothing else: no tables, no devices, no GUI. A kind registers itself
   the first time it is used, and built-in members of a kind never pass
   through the loader at all. */
class PluginKind
{
public:
	virtual ~PluginKind() {}

	virtual const char *Name() const = 0;
	// Under the plugin root; modules are <root>/<subdirectory>/*<suffix><ext>
	// and one level of directories below that, where the extension is the
	// host's loadable module extension and never part of the suffix.
	virtual const char *Subdirectory() const = 0;
	virtual const char *Suffix() const = 0;
	virtual const char *EntrySymbol() const = 0;

	// The resolved entry; return true to keep the module loaded.
	virtual bool Accept(void *entry, const std::string &path) = 0;
};

//////////////////////////////////////////////////////////

class PluginLoader
{
public:
	static PluginLoader *Get() { if(!m_Singleton) m_Singleton=new PluginLoader; return m_Singleton; }
	static void         PackUpAndGoHome() { if(m_Singleton) delete m_Singleton; m_Singleton=NULL; }

	void                RegisterKind(PluginKind *kind);
	unsigned            Load(PluginKind &kind, const std::string &root);
	unsigned            LoadAll(const std::string &root);
	void                UnloadAll();

private:

	PluginLoader();
	~PluginLoader();
	bool LoadModule(PluginKind &kind, const std::string &path);

	std::vector<PluginKind*> m_Kinds;
	std::vector<void*> m_Modules;
	static PluginLoader *m_Singleton;
};

}

#endif
