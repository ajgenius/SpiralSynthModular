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


#define SCRATCH_DEVICE "/dev/ttyS0"

#include "MousePlugin.h"
#include "MousePluginGUI.h"
#include "SpiralIcon.xpm"

extern "C" {

SpiralPlugin* SpiralPlugin_CreateInstance() { return new MousePlugin; }

char** SpiralPlugin_GetIcon() { return SpiralIcon_xpm; }

int SpiralPlugin_GetID() { return 300; }

string SpiralPlugin_GetGroupName() { return "InputOutput"; }

}

///////////////////////////////////////////////////////

MousePluginSingleton* MousePluginSingleton::m_Singleton = NULL;

int MousePlugin::m_RefCount=0;

MousePluginSingleton::MousePluginSingleton() {
     scr = new scratch(SCRATCH_DEVICE); // create scratch object
}

MousePluginSingleton::~MousePluginSingleton() {
     if (scr!=NULL) delete scr;
}

///////////////////////////////////////////////////////

MousePlugin::MousePlugin():
m_Port ('0'),
m_Data (0.0)
{
	m_RefCount++;
	m_PluginInfo.Name = "Mouse";
	m_PluginInfo.Width = 100;
	m_PluginInfo.Height = 140;
	m_PluginInfo.NumInputs = 1;
	m_PluginInfo.NumOutputs = 1;
	m_PluginInfo.PortTips.push_back ("Trigger");
	m_PluginInfo.PortTips.push_back ("Output");
        m_AudioCH->Register ("Port", &m_Port);
        m_Version = 2;
}

MousePlugin::~MousePlugin() {
     m_RefCount--;
     if (m_RefCount==0) {
        MousePluginSingleton::PackUpAndGoHome();
     }
}

PluginInfo &MousePlugin::Initialise (const HostInfo *Host) {
           PluginInfo& Info = SpiralPlugin::Initialise (Host);
           m_AudioCH->Register ("Data", &m_Data, ChannelHandler::OUTPUT);
           return Info;
}

SpiralGUIType *MousePlugin::CreateGUI() {
      return new MousePluginGUI (m_PluginInfo.Width, m_PluginInfo.Height, this, m_AudioCH, m_HostInfo);
}

void MousePlugin::ExecuteCommands () {
  if (m_AudioCH->IsCommandWaiting ()) {
    switch (m_AudioCH->GetCommand()) {
      case (SETPORT) : // do something
                     break;
    }
  }
}

void MousePlugin::Execute() {
     float trigger = 1.0;
     if (GetOutputBuf(0)) {
        if (InputExists(0)) {
           trigger = GetInput(0,0);
        }
        char c = MousePluginSingleton::Get()->getScr()->getData();
        if (c!=0x0) {
           float val;
           int ch = (int)c;
           val = (float)(ch / 127.00);
           if (val>1) val=1;
           if (val<-1) val=-1;
           if (trigger>0) {
              GetOutputBuf(0)->Set (val);
              m_Data = val;
           } else {
              GetOutputBuf(0)->Set (m_Data);
           }
        } else {
            GetOutputBuf(0)->Set (m_Data);
        }
    }
}

void MousePlugin::StreamOut (ostream &s) {
  s << m_Version << " " << m_Port;
}

void MousePlugin::StreamIn (istream &s) {
  int Version;
  s >> Version;
  if (Version > 1) s >> m_Port;
}
