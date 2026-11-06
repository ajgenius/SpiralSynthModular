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

#ifndef SPIRAL_PLUGIN_MANAGER_H
#define SPIRAL_PLUGIN_MANAGER_H

#include <string>
#include <vector>

namespace Spiral
{
namespace Core
{

class PluginKind
{
public:
	virtual ~PluginKind() {}

	virtual const char *Name() const = 0;
	virtual const char *Subdirectory() const = 0;
	virtual const char *Suffix() const = 0;
	virtual const char *EntrySymbol() const = 0;

	virtual bool Claims(const std::string &file) const;

	virtual bool Accept(void *entry, const std::string &path) = 0;

	virtual bool CanUnload() const { return true; }
	virtual void Release(void *entry, const std::string &path) {}
};

//////////////////////////////////////////////////////////

class PluginManager
{
public:
	static PluginManager *Get() { if(!m_Singleton) m_Singleton=new PluginManager; return m_Singleton; }
	static void         PackUpAndGoHome();

	void                RegisterKind(PluginKind *kind);
	bool                UnregisterKind(PluginKind *kind);
	unsigned            Load(PluginKind &kind, const std::string &root);
	unsigned            LoadAll(const std::string &root);
	bool                UnloadAll();

private:

	PluginManager();
	~PluginManager();
	bool LoadModule(PluginKind &kind, const std::string &path);

	std::vector<PluginKind*> m_Kinds;
	struct Module
	{
		PluginKind *Kind;
		void *Handle, *Entry;
		std::string Path;
	};
	void ReleaseModule(const Module &module);
	std::vector<Module> m_Modules;
	static PluginManager *m_Singleton;
};

}
}

#endif
