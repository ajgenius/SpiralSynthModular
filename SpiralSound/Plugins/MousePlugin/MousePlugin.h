/*  MousePlugin
 *  Copyleft (C) 2002 Dan Bethell <dan@pawfal.org>
 *                    Dave Griffiths <dave@pawfal.org>
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

#ifndef MousePlugin_H
#define MousePlugin_H

#include "../SpiralPlugin.h"
#include "scratch.h"

class MousePluginSingleton {
   public:
      static MousePluginSingleton *Get() {
             if (!m_Singleton) m_Singleton = new MousePluginSingleton;
             return m_Singleton;
      }
      static void PackUpAndGoHome() {
             if (m_Singleton)  {
                delete m_Singleton;
                m_Singleton=NULL;
             }
      }
      ~MousePluginSingleton();
      scratch *getScr() { return scr; }
   private:
      MousePluginSingleton();
      scratch *scr;
      static MousePluginSingleton* m_Singleton;
      int count;
};

class MousePlugin : public SpiralPlugin {
   public:
      MousePlugin();
      virtual ~MousePlugin();
      virtual PluginInfo& Initialise (const HostInfo *Host);
      virtual SpiralGUIType* CreateGUI();
      char GetPort (void) { return m_Port; }
      virtual void Execute();
      virtual void ExecuteCommands();
      virtual void StreamOut (ostream &s);
      virtual void StreamIn (istream &s);
      enum GUICommands {NONE, SETPORT};
   private:
      static int m_RefCount;
      char m_Port;
      float m_Data;
      friend istream &operator>> (istream &s, MousePlugin &o);
      friend ostream &operator<< (ostream &s, MousePlugin &o);
};

istream &operator>> (istream &s, MousePlugin &o);
ostream &operator<< (ostream &s, MousePlugin &o);

#endif
