/*  SpiralSynthModular
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

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/fl_file_chooser.h>
#include <FL/Fl_Box.h>
#include <FL/Fl_Tooltip.h>
#include "SpiralSynthModular.h"
#include "SpiralSound/PluginManager.h"
#include "SpiralSound/SpiralInfo.h"
#include "SpiralSound/Plugins/SpiralPluginGUI.h"
#include "GUI/SSM.xpm"
#include "GUI/load.xpm"
#include "GUI/save.xpm"
#include "GUI/new.xpm"
#include "GUI/options.xpm"
#include "GUI/comment.xpm"
#include "GUI/Widgets/PawfalYesNo.h"

//#define DEBUG_PLUGINS
//#define DEBUG_STREAM

const static string LABEL = "SpiralSynthModular "+VER_STRING;
static string TITLEBAR;

static const int FILE_VERSION = 4;
static int Numbers[512];

static const int MAIN_WIDTH     = 700;
static const int MAIN_HEIGHT    = 600;
static const int SLIDER_WIDTH   = 15;
static const int ICON_DEPTH     = 3;
static const int COMMENT_ID     = -1;

using namespace std;

map<int,DeviceWin*> SynthModular::m_DeviceWinMap;
bool SynthModular::m_CallbackUpdateMode = false;
bool SynthModular::m_BlockingOutputPluginIsReady = false;

//////////////////////////////////////////////////////////

DeviceWin::~DeviceWin()
{
}

//////////////////////////////////////////////////////////

SynthModular::SynthModular():
m_Frozen(false),
m_NextID(0)
{
	/* Shared Audio State Information  */
	m_Info.BUFSIZE = SpiralInfo::BUFSIZE;
	m_Info.SAMPLERATE = SpiralInfo::SAMPLERATE;
	m_Info.PAUSED = false;

	/* obsolete - REMOVE SOON  */
	m_Info.FRAGSIZE = SpiralInfo::FRAGSIZE;
	m_Info.FRAGCOUNT = SpiralInfo::FRAGCOUNT;
	m_Info.OUTPUTFILE = SpiralInfo::OUTPUTFILE;
	m_Info.MIDIFILE = SpiralInfo::MIDIFILE;
	m_Info.POLY = SpiralInfo::POLY;

	/* Shared GUI Preferences Information  */
        m_Info.GUI_COLOUR = SpiralInfo::GUI_COLOUR;
        m_Info.SCOPE_BG_COLOUR = SpiralInfo::SCOPE_BG_COLOUR;
        m_Info.SCOPE_FG_COLOUR = SpiralInfo::SCOPE_FG_COLOUR;
        m_Info.SCOPE_SEL_COLOUR = SpiralInfo::SCOPE_SEL_COLOUR;
        m_Info.SCOPE_IND_COLOUR = SpiralInfo::SCOPE_IND_COLOUR;
        m_Info.SCOPE_MRK_COLOUR = SpiralInfo::SCOPE_MRK_COLOUR;
        m_Info.GUICOL_Device = SpiralInfo::GUICOL_Device;
        m_Info.GUIDEVICE_Box = SpiralInfo::GUIDEVICE_Box;

        for (int n=0; n<512; n++) Numbers[n]=n;

	m_CH.Register("Frozen",&m_Frozen);
}

//////////////////////////////////////////////////////////

SynthModular::~SynthModular()
{
	ClearUp();
	PluginManager::Get()->PackUpAndGoHome();
	system("rm -f ___temp.ssmcopytmp");
}

//////////////////////////////////////////////////////////

void SynthModular::ClearUp()
{
	FreezeAll();

	for(map<int,DeviceWin*>::iterator i=m_DeviceWinMap.begin();
		i!=m_DeviceWinMap.end(); i++)
	{
		//Stop processing of audio if any
		if (i->second->m_Device)
		{
			if (i->second->m_Device->Kill());
		}
		i->second->m_DeviceGUI->Clear();

		if (i->second->m_DeviceGUI->GetPluginWindow())
		{
			i->second->m_DeviceGUI->GetPluginWindow()->hide();
		}

		//Delete Device
		delete i->second->m_Device;
		i->second->m_Device=NULL;
	}

	m_Canvas->Clear();
	m_DeviceWinMap.clear();
	m_NextID=0;

	ThawAll();
}

//////////////////////////////////////////////////////////
void SynthModular::Update()
{
	m_CH.UpdateDataNow();

	if (m_Frozen) return;

	// for all the plugins
	for(map<int,DeviceWin*>::iterator i=m_DeviceWinMap.begin();
		i!=m_DeviceWinMap.end(); i++)
	{
		if (i->second->m_Device && i->second->m_Device->IsDead())
		{
			//Delete Device
			delete i->second->m_Device;
			i->second->m_Device=NULL;

			//Erase Device from DeviceWinMap
			m_DeviceWinMap.erase(i);
		}
		else if (i->second->m_Device) // if it's not a comment
		{
			#ifdef DEBUG_PLUGINS
			cerr<<"Updating channelhandler of plugin "<<i->second->m_PluginID<<endl;
			#endif

			// updates the data from the gui thread, if it's not blocking
 			i->second->m_Device->UpdateChannelHandler();

			#ifdef DEBUG_PLUGINS
			cerr<<"Finished updating"<<endl;
			#endif

			// If this is an audio device see if we always need to ProcessAudio here
			if ((!m_ResetingAudioThread))
			{
				if (i->second->m_Device->IsAudioDriver())
				{
					AudioDriver *driver = ((AudioDriver *)i->second->m_Device);

					if (driver->ProcessType() == AudioDriver::ALWAYS)
					{
						driver->ProcessAudio();
					}
				}

				// run any commands we've received from the GUI's
				i->second->m_Device->ExecuteCommands();
			}
		}
	}

	// run the plugins (only ones connected to anything)
	list<int> ExecutionOrder = m_Canvas->GetGraph()->GetSortedList();
	for (list<int>::reverse_iterator i=ExecutionOrder.rbegin();
		 i!=ExecutionOrder.rend(); i++)
	{
		// use the graphsort order to remove internal latency
		map<int,DeviceWin*>::iterator di=m_DeviceWinMap.find(*i);
		if (di!=m_DeviceWinMap.end() && di->second->m_Device  && (! di->second->m_Device->IsDead()) && (!m_Info.PAUSED || m_ResetingAudioThread))
		{
			#ifdef DEBUG_PLUGINS
			cerr<<"Executing plugin "<<di->second->m_PluginID<<endl;
			#endif

			if (m_ResetingAudioThread)
			{
				di->second->m_Device->Reset();
			}
			else
			{
				di->second->m_Device->Execute();

				// If this is an audio device see if we need to ProcessAudio here
				if (di->second->m_Device->IsAudioDriver())
				{
					AudioDriver *driver = ((AudioDriver *)di->second->m_Device);

					if (driver->ProcessType() == AudioDriver::MANUAL)
					{
						driver->ProcessAudio();
					}
				}
			}

			#ifdef DEBUG_PLUGINS
			cerr<<"Finished executing"<<endl;
			#endif
		}
	}

	//we can safely turn this off here
	m_ResetingAudioThread = false;
}

//////////////////////////////////////////////////////////

void SynthModular::UpdatePluginGUIs()
{
	// see if any need deleting
	for (map<int,DeviceWin*>::iterator i=m_DeviceWinMap.begin();
		 i!=m_DeviceWinMap.end(); i++)
	{
		if (i->second->m_DeviceGUI && i->second->m_DeviceGUI->GetPluginWindow())
		{
			SpiralPluginGUI *GUI=(SpiralPluginGUI *)i->second->m_DeviceGUI->GetPluginWindow();
			GUI->Update();
		}

		if (i->second->m_DeviceGUI && i->second->m_DeviceGUI->Killed())
		{
			bool erase = true;

			//Stop processing of audio if any
			if (i->second->m_Device)
			{
				if (i->second->m_Device->Kill());
				erase = false;
			}

			//Clear GUI Device
			i->second->m_DeviceGUI->Clear();

			// Hide Device GUI FIRST
			if (i->second->m_DeviceGUI->GetPluginWindow())
			{
				i->second->m_DeviceGUI->GetPluginWindow()->hide();
			}

			//Remove Device GUI from canvas
			m_Canvas->RemoveDevice(i->second->m_DeviceGUI);

			//Delete Device GUI - must delete here or sometimes plugin will randomly crash
			delete i->second->m_DeviceGUI;
			i->second->m_DeviceGUI = NULL;

			//Erase from winmap if no audio to do it
			if (erase)
				m_DeviceWinMap.erase(i);
		}
	}

	m_Canvas->Poll();

	if (m_HostNeedsUpdate)
	{
		cout << "Updating SampleRate to: " << SpiralInfo::SAMPLERATE << " and Buffer Size to: " << SpiralInfo::BUFSIZE << " to match current Audio Driver." << endl;
		UpdateHostInfo();
		m_HostNeedsUpdate = false;
	}
}

//////////////////////////////////////////////////////////

SpiralWindowType *SynthModular::CreateWindow()
{
	m_TopWindow = new SpiralWindowType(MAIN_WIDTH, MAIN_HEIGHT, LABEL.c_str());
        m_TopWindow->user_data((void*)(this));
	//m_TopWindow->resizable(m_TopWindow);
        m_MainMenu = new Fl_Menu_Bar (0, 0, MAIN_WIDTH, 20, "");
        m_MainMenu->user_data((void*)(this));
        m_MainMenu->box(FL_PLASTIC_UP_BOX);
        m_MainMenu->textsize (10);
        m_MainMenu->add ("File/New", 0, cb_New, (void*)(this), FL_MENU_DIVIDER);
        m_MainMenu->add ("File/Load", 0, cb_Load, (void*)(this), 0);
        m_MainMenu->add ("File/Save As", 0, cb_Save, (void*)(this), 0);
        m_MainMenu->add ("File/Merge", 0, cb_Merge, (void*)(this), FL_MENU_DIVIDER);
        m_MainMenu->add ("File/Exit", 0, cb_Close, (void*)(this), 0);
        m_MainMenu->add ("Edit/Cut", 0, cb_Cut, (void*)(this), 0);
        m_MainMenu->add ("Edit/Copy", 0, cb_Copy, (void*)(this), 0);
        m_MainMenu->add ("Edit/Paste", 0, cb_Paste, (void*)(this), 0);
        m_MainMenu->add ("Edit/Delete", 0, cb_Delete, (void*)(this), FL_MENU_DIVIDER);
        //m_MainMenu->add ("Edit/Toolbars/Plugins", 0, cb_Undefined, (void*)(this), 0);
        //m_MainMenu->add ("Edit/Toolbars/Function", 0, cb_Undefined, (void*)(this), 0);
        m_MainMenu->add ("Edit/Options", 0, cb_Options, (void*)(this), 0);
        m_MainMenu->add ("Plugins/dummy", 0, NULL, NULL, 0);
        m_MainMenu->add ("Audio/Pause", 0, cb_PlayPause, NULL, 0);
        m_MainMenu->add ("Audio/Reset", 0, cb_Reset, NULL, 0);
        //m_MainMenu->add ("Help/Plugins/dummy", 0, NULL, NULL, 0);
        //m_MainMenu->add ("Help/Credits", 0, NULL, (void*)(this), 0);
        //m_MainMenu->add ("Help/About", 0, NULL, (void*)(this), 0);
        m_TopWindow->add (m_MainMenu);
	int but = 50;
        int ToolbarHeight = but + 0;
        m_Topbar = new Fl_Pack (0, 20, MAIN_WIDTH, ToolbarHeight, "");
        m_Topbar->user_data((void*)(this));
       	m_Topbar->type(FL_HORIZONTAL);
	m_Topbar->color(SpiralInfo::GUICOL_Button);
        m_TopWindow->add(m_Topbar);

        m_ToolbarPanel = new Fl_Pack (0, 20, but*6, ToolbarHeight, "");
        m_ToolbarPanel->user_data((void*)(this));
       	m_ToolbarPanel->type(FL_VERTICAL);
	m_ToolbarPanel->color(SpiralInfo::GUICOL_Button);
        m_Topbar->add(m_ToolbarPanel);

        m_Toolbar = new Fl_Pack (0, 20, but*6, but, "");
        m_Toolbar->user_data((void*)(this));
       	m_Toolbar->type(FL_HORIZONTAL);
	m_Toolbar->color(SpiralInfo::GUICOL_Button);
        m_ToolbarPanel->add(m_Toolbar);

        m_Load = new Fl_Button (0, 0, but, but, "");
        m_Load->user_data ((void*)(this));
	Fl_Pixmap *tPix = new Fl_Pixmap(load_xpm);
	m_Load->image(tPix->copy());
	delete tPix;
        m_Load->type(0);
	m_Load->box(FL_PLASTIC_UP_BOX);
	m_Load->color(SpiralInfo::GUICOL_Button);
	m_Load->selection_color(SpiralInfo::GUICOL_Tool);
        m_Load->labelsize (1);
        m_Load->tooltip("Load a patch file");
	m_Load->callback((Fl_Callback*)cb_Load);
	m_Toolbar->add(m_Load);

	m_Save = new Fl_Button(0, 0, but, but, "");
        m_Save->user_data ((void*)(this));
	tPix = new Fl_Pixmap(save_xpm);
	m_Save->image(tPix->copy());
	delete tPix;
        m_Save->type(0);
	m_Save->box(FL_PLASTIC_UP_BOX);
	m_Save->color(SpiralInfo::GUICOL_Button);
	m_Save->selection_color(SpiralInfo::GUICOL_Tool);
        m_Save->labelsize (1);
 	m_Save->tooltip("Save a patch file");
	m_Save->callback((Fl_Callback*)cb_Save);
	m_Toolbar->add(m_Save);

	m_New = new Fl_Button(0, 0, but, but, "");
        m_New->user_data ((void*)(this));
	tPix = new Fl_Pixmap(new_xpm);
	m_New->image(tPix->copy());
	delete tPix;
        m_New->type(0);
	m_New->box(FL_PLASTIC_UP_BOX);
	m_New->color(SpiralInfo::GUICOL_Button);
	m_New->selection_color(SpiralInfo::GUICOL_Tool);
  	m_New->labelsize (1);
        m_New->tooltip("New patch");
	m_New->callback((Fl_Callback*)cb_New);
	m_Toolbar->add(m_New);

	m_Options = new Fl_Button(0, 0, but, but, "");
        m_Options->user_data ((void*)(this));
	tPix = new Fl_Pixmap(options_xpm);
	m_Options->image(tPix->copy());
	delete tPix;
        m_Options->type(0);
	m_Options->box(FL_PLASTIC_UP_BOX);
	m_Options->color(SpiralInfo::GUICOL_Button);
	m_Options->selection_color(SpiralInfo::GUICOL_Tool);
 	m_Options->labelsize (1);
        m_Options->tooltip("Options");
	m_Options->callback((Fl_Callback*)cb_Options);
	m_Toolbar->add(m_Options);

	m_NewComment = new Fl_Button(0, 0, but, but, "");
	tPix = new Fl_Pixmap(comment_xpm);
	m_NewComment->image(tPix->copy());
	delete tPix;
        m_NewComment->type(0);
	m_NewComment->box(FL_PLASTIC_UP_BOX);
	m_NewComment->color(SpiralInfo::GUICOL_Button);
	m_NewComment->selection_color(SpiralInfo::GUICOL_Tool);
        m_NewComment->labelsize (1);
        m_NewComment->tooltip("New comment");
	m_NewComment->callback((Fl_Callback*)cb_NewComment);
	m_Toolbar->add(m_NewComment);

        m_PlayResetGroup = new Fl_Pack (0, 0, but, but, "");
	m_PlayResetGroup->color(SpiralInfo::GUICOL_Button);
        m_Toolbar->add(m_PlayResetGroup);

	m_PlayPause = new Fl_Button(0, 0, but, but/2, "@||");
        m_PlayPause->user_data((void*)(this));
        m_PlayPause->type(0);
	m_PlayPause->box(FL_PLASTIC_UP_BOX);
	m_PlayPause->color(SpiralInfo::GUICOL_Button);
	m_PlayPause->selection_color(SpiralInfo::GUICOL_Tool);
        m_PlayPause->labelsize (10);
        m_PlayPause->tooltip("Pause");
	m_PlayPause->callback((Fl_Callback*)cb_PlayPause);
	m_PlayResetGroup->add(m_PlayPause);

	m_Reset = new Fl_Button(0, 0, but, but/2, "Reset");
	m_Reset->box(FL_PLASTIC_UP_BOX);
	m_Reset->color(SpiralInfo::GUICOL_Button);
        m_Reset->user_data((void*)(this));
	m_Reset->selection_color(SpiralInfo::GUICOL_Tool);
        m_Reset->labelsize (10);
        m_Reset->tooltip("Reset Audio State of all Plugins");
	m_Reset->callback((Fl_Callback*)cb_Reset);
	m_PlayResetGroup->add(m_Reset);

        m_GroupFiller = new Fl_Group (0, 0, 0, ToolbarHeight, "");
	m_GroupFiller->color(SpiralInfo::GUICOL_Button);
	m_Topbar->add (m_GroupFiller);

       	m_GroupTab = new Fl_Tabs (0, 0, MAIN_WIDTH-m_GroupFiller->w()-but*6, ToolbarHeight, "");
        m_GroupTab->user_data ((void*)(this));
	m_GroupTab->box(FL_PLASTIC_DOWN_BOX);
	m_GroupTab->color(SpiralInfo::GUICOL_Button);
        m_GroupTab->callback((Fl_Callback*)cb_GroupTab);
	m_Topbar->add (m_GroupTab);

       	m_Topbar->resizable(m_GroupTab);

        /////////////////

        ToolbarHeight += 20; // Stretch this a bit to allow room for the menu-bar too.
	m_CanvasScroll = new Fl_Scroll (0, ToolbarHeight, MAIN_WIDTH, MAIN_HEIGHT-ToolbarHeight, "");
        m_TopWindow->add(m_CanvasScroll);
	m_TopWindow->resizable(m_CanvasScroll);

	m_Canvas = new Fl_Canvas(-5000, -5000, 10000, 10000, "");
        m_Canvas->user_data ((void*)(this));
        m_Canvas->type(1);
	m_Canvas->box(FL_FLAT_BOX);
        m_Canvas->labeltype(FL_ENGRAVED_LABEL);
        m_Canvas->align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE);
	m_Canvas->color(SpiralInfo::GUICOL_Canvas);
	m_Canvas->user_data((void*)(this));
	m_Canvas->SetConnectionCallback((Fl_Callback*)cb_Connection);
	m_Canvas->SetUnconnectCallback((Fl_Callback*)cb_Unconnect);
	m_Canvas->SetAddDeviceCallback((Fl_Callback*)cb_NewDeviceFromCanvasMenu);
	m_Canvas->SetCutDeviceGroupCallback((Fl_Callback*)cb_Cut);
	m_Canvas->SetCopyDeviceGroupCallback((Fl_Callback*)cb_Copy);
	m_Canvas->SetPasteDeviceGroupCallback((Fl_Callback*)cb_Paste);
        m_Canvas->SetMergePatchCallback((Fl_Callback*)cb_Merge);

	m_CanvasScroll->add(m_Canvas);

	m_SettingsWindow = new SettingsWindow;
	m_SettingsWindow->RegisterApp(this);

	return m_TopWindow;
}

//////////////////////////////////////////////////////////

vector<string> SynthModular::BuildPluginList (const string &Path) {
	// Scan plugin path for plugins.
	DIR *dp;
	struct dirent *ep;
	struct stat sb;
	void *handle;
	string fullpath;
	const char *path = Path.c_str();
	vector<string> ret;

	dp = opendir(path);
	if (!dp) {
	   cerr << "WARNING: Could not open path " << path << endl;
	}
	else {
		while ((ep = readdir(dp))) {
			// Need full path
			fullpath = path;
			fullpath.append(ep->d_name);

                        // Stat file to get type
			if (!stat(fullpath.c_str(), &sb)) {
				// We only want regular files
				if (S_ISREG(sb.st_mode))  {
                                        // We're not fussed about resolving symbols yet, since we are just
                                        // checking if it's a DLL.
					handle = dlopen(fullpath.c_str(), RTLD_LAZY);
					if (!handle) {
						cerr << "WARNING: File " << path << ep->d_name
							<< " could not be examined" << endl;
						cerr << "dlerror() output:" << endl;
						cerr << dlerror() << endl;
					}
					else {
						// It's a DLL. Add name to list
						ret.push_back(ep->d_name);
					}
                                }
                        }
		}
	}
        return ret;
}

void SynthModular::LoadPlugins (string pluginPath) {
     int Width  = 35;
     int Height = 35;
     int SWidth  = 256;
     int SHeight = 256;
     Fl_Pixmap pic (SSM_xpm);
     Fl_Double_Window* Splash = new Fl_Double_Window ((Fl::w()/2) - (SWidth/2), (Fl::h()/2) - (SHeight/2),
                                                       SWidth, SHeight, "SSM");
     Splash->border(0);
     Fl_Box* pbut = new Fl_Box (0, 8, SWidth, SHeight, "");
     pbut->box (FL_NO_BOX);
     pic.label (pbut);
     Fl_Box *splashtext = new Fl_Box (5, SHeight-20, 200, 20, "Loading...");
     splashtext->labelsize (10);
     splashtext->box (FL_NO_BOX);
     splashtext->align (FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
     Splash->add (pbut);
     Splash->add (splashtext);
     Splash->show();
     int ID=-1;
     vector<string> PluginVector;
     if (SpiralInfo::USEPLUGINLIST) PluginVector = SpiralInfo::PLUGINVEC;
     else {
        if (pluginPath.empty()) PluginVector = BuildPluginList (SpiralInfo::PLUGIN_PATH);
        else {
           string::iterator i = pluginPath.end() - 1;
           if (*i != '/') pluginPath += '/';
           PluginVector = BuildPluginList (pluginPath);
        }
     }
     for (vector<string>::iterator i=PluginVector.begin(); i!=PluginVector.end(); i++) {
         string Fullpath;
         if (pluginPath=="") Fullpath=SpiralInfo::PLUGIN_PATH+*i;
         else Fullpath = pluginPath + *"/" + *i;
         ID = PluginManager::Get()->LoadPlugin (Fullpath.c_str());
         if (ID!=PluginError) {
            #ifdef DEBUG_PLUGINS
            cerr << ID << " = Plugin [" << *i << "]" << endl;
            #endif
            Fl_ToolButton *NewButton = new Fl_ToolButton (0, 0, Width, Height, "");
            // we can't set user data, because the callback uses it
            // NewButton->user_data ((void*)(this));
            NewButton->labelsize (1);
            Fl_Pixmap *tPix = new Fl_Pixmap (PluginManager::Get()->GetPlugin(ID)->GetIcon());
            NewButton->image(tPix->copy(tPix->w(),tPix->h()));
            delete tPix;
            string GroupName = PluginManager::Get()->GetPlugin(ID)->GetGroupName();
            Fl_Pack* the_group=NULL;
            // find or create this group, and add an icon
            map<string,Fl_Pack*>::iterator gi = m_PluginGroupMap.find (GroupName);
            if (gi == m_PluginGroupMap.end()) {
               the_group = new Fl_Pack (m_GroupTab->x(), 16, m_GroupTab->w(), m_GroupTab->h() - 15, GroupName.c_str());
               the_group->type(FL_HORIZONTAL);
               the_group->labelsize(8);
               the_group->color(SpiralInfo::GUICOL_Button);
               the_group->user_data((void*)(this));
               //m_GroupTab->add(the_group);
               m_GroupTab->value(the_group);
               m_PluginGroupMap[GroupName]=the_group;
            }
            else the_group = gi->second;
            NewButton->type (0);
            NewButton->box (FL_NO_BOX);
            NewButton->down_box (FL_NO_BOX);
            //NewButton->color(SpiralInfo::GUICOL_Button);
            //NewButton->selection_color(SpiralInfo::GUICOL_Button);
            the_group->add (NewButton);
            string PluginName=*i;
            // find the first slash, if there is one, and get rid of everything before and including it
            unsigned int p = PluginName.find ('/');
            if (p < PluginName.length()) PluginName.erase (0, p);
            // find last . and get rid of everything after and including it
            p = PluginName.rfind ('.');
            unsigned int l = PluginName.length ();
            if (p < l) PluginName.erase (p, l);
            NewButton->tooltip (PluginName.c_str());
            // Slashes have significance to the menu widgets, remove them from the GroupName
            while ((p = GroupName.find ('/')) < PluginName.length())
                  GroupName = GroupName.replace (p, 1, " and ");
            string MenuEntry = "Plugins/" + GroupName + "/" + PluginName;
            m_MainMenu->add (MenuEntry.c_str(), 0, cb_NewDeviceFromMenu, &Numbers[ID], 0);
            //MenuEntry = "Help/" + MenuEntry;
            //m_MainMenu->add (MenuEntry.c_str(), 0, NULL, &Numbers[ID], 0);

            // andy preston
            // my next step would be to add the plugins to Andrew's right-click menu
            // to free up the middle button

            m_Canvas->AddPluginName (PluginName, PluginManager::Get()->GetPlugin(ID)->ID);


            // this overwrites the widget's user_data with that specified for the callback
            // so we can't use it for other purposes
            NewButton->callback ((Fl_Callback*)cb_NewDevice, &Numbers[ID]);
            NewButton->show();
            m_DeviceVec.push_back (NewButton);
            the_group->redraw();
            // m_NextPluginButton++;
            Fl::check();
            splashtext->label (PluginName.c_str());
            Splash->redraw();
         }
     }
     map<string,Fl_Pack*>::iterator PlugGrp;
     for (PlugGrp = m_PluginGroupMap.begin(); PlugGrp!= m_PluginGroupMap.end(); ++PlugGrp) {
         m_GroupTab->add (PlugGrp->second);
         PlugGrp->second->add (new Fl_Box (0, 0, 600, 100, ""));
     }
     // try to show the SpiralSound group
     PlugGrp = m_PluginGroupMap.find("SpiralSound");
     // can't find it - show the first plugin group
     if (PlugGrp==m_PluginGroupMap.end()) PlugGrp=m_PluginGroupMap.begin();
     m_GroupTab->value(PlugGrp->second);
     bool found_dummy;
     int i;
     do {
        found_dummy = false;
        for (i=0; i<m_MainMenu->size(); i++) {
            if (m_MainMenu->text (i) != NULL) {
               found_dummy = (strcmp ("dummy", m_MainMenu->text (i)) == 0);
               if (found_dummy) break;
            }
        }
        if (found_dummy) m_MainMenu->remove (i);
     } while (found_dummy);
     Splash->hide();
     delete Splash;
}

//////////////////////////////////////////////////////////

DeviceGUIInfo SynthModular::BuildDeviceGUIInfo(PluginInfo &PInfo)
{
	DeviceGUIInfo Info;
	int Height=50;

	// tweak the size if we have too many ins/outs
	if (PInfo.NumInputs>4 || PInfo.NumOutputs>4)
	{
		if (PInfo.NumInputs<PInfo.NumOutputs)
		{
			Height=PInfo.NumOutputs*10+5;
		}
		else
		{
			Height=PInfo.NumInputs*10+5;
		}
	}

	// Make the guiinfo struct
	Info.XPos       = 0;
	Info.YPos       = 0;
	Info.Width      = 40;
	Info.Height     = Height;
	Info.NumInputs  = PInfo.NumInputs;
	Info.NumOutputs = PInfo.NumOutputs;
	Info.Name       = PInfo.Name;
	Info.PortTips   = PInfo.PortTips;
	Info.PortTypes  = PInfo.PortTypes;

	return Info;
}

//////////////////////////////////////////////////////////

DeviceWin* SynthModular::NewDeviceWin(int n, int x, int y)
{
	DeviceWin *nlw = new DeviceWin;
	const HostsideInfo* Plugin=PluginManager::Get()->GetPlugin(n);

	if (!Plugin) return NULL;

	nlw->m_Device=Plugin->CreateInstance();

	if (!nlw->m_Device) return NULL;

        nlw->m_Device->SetBlockingCallback(cb_Blocking);
	nlw->m_Device->SetUpdateCallback(cb_Update);
	nlw->m_Device->SetParent((void*)this);

	if ( nlw->m_Device->IsAudioDriver() )
	{
		AudioDriver *driver = ((AudioDriver*)nlw->m_Device);
		driver->SetChangeBufferAndSampleRateCallback(cb_ChangeBufferAndSampleRate);
	}

	PluginInfo PInfo    = nlw->m_Device->Initialise(&m_Info);
	SpiralGUIType *temp = nlw->m_Device->CreateGUI();
	Fl_Pixmap *Pix      = new Fl_Pixmap(Plugin->GetIcon());
	nlw->m_PluginID     = n;

	if (temp) temp->position(x+10,y);

	DeviceGUIInfo Info=BuildDeviceGUIInfo(PInfo);

	Info.XPos       = x; //TOOLBOX_WIDTH+(rand()%400);
	Info.YPos       = y; //rand()%400;

	nlw->m_DeviceGUI = new Fl_DeviceGUI(Info, temp, Pix, nlw->m_Device->IsTerminal());
	Fl_Canvas::SetDeviceCallbacks(nlw->m_DeviceGUI, m_Canvas);
	m_Canvas->add(nlw->m_DeviceGUI);
	m_Canvas->redraw();

	return nlw;
}

//////////////////////////////////////////////////////////

void SynthModular::AddDevice(int n, int x=-1, int y=-1)
{
	//cerr<<"Adding "<<m_NextID<<endl;

	if (x==-1)
	{
		x = m_CanvasScroll->x()+50;
		y = m_CanvasScroll->y()+50;
	}

	DeviceWin* temp = NewDeviceWin(n,x,y);
	if (temp)
	{
		int ID=m_NextID++;
		//cerr<<"adding device "<<ID<<endl;
		temp->m_DeviceGUI->SetID(ID);
		temp->m_Device->SetUpdateInfoCallback(ID,cb_UpdatePluginInfo);
		m_DeviceWinMap[ID]=temp;
	}
}

//////////////////////////////////////////////////////////

DeviceWin* SynthModular::NewComment(int n, int x=-1, int y=-1)
{
	DeviceWin *nlw = new DeviceWin;

	if (x==-1)
	{
		x = m_CanvasScroll->x()+50;
		y = m_CanvasScroll->y()+50;
	}

	nlw->m_Device=NULL;
	nlw->m_PluginID  = COMMENT_ID;

	DeviceGUIInfo Info;

	Info.XPos       = x;
	Info.YPos       = y;
	Info.Width      = 50;
	Info.Height     = 20;
	Info.NumInputs  = 0;
	Info.NumOutputs = 0;
	Info.Name = "";

	nlw->m_DeviceGUI = new Fl_CommentGUI(Info, NULL, NULL);

	Fl_Canvas::SetDeviceCallbacks(nlw->m_DeviceGUI, m_Canvas);
	m_Canvas->add(nlw->m_DeviceGUI);
	m_Canvas->redraw();

	return nlw;
}

//////////////////////////////////////////////////////////

void SynthModular::AddComment(int n)
{
	//cerr<<"Adding "<<m_NextID<<endl;
	DeviceWin* temp = NewComment(n);
	if (temp)
	{
		int ID=m_NextID++;
		//cerr<<"adding comment "<<ID<<endl;
		temp->m_DeviceGUI->SetID(ID);
		m_DeviceWinMap[ID]=temp;
	}
}

//////////////////////////////////////////////////////////

void SynthModular::cb_ChangeBufferAndSampleRate_i(long int NewBufferSize, long int NewSamplerate)
{
	if (SpiralInfo::BUFSIZE != NewBufferSize)
	{
		// update the settings
		SpiralInfo::BUFSIZE    = NewBufferSize;
		m_HostNeedsUpdate = true;
	}

	if (SpiralInfo::SAMPLERATE != NewSamplerate)
	{
		SpiralInfo::SAMPLERATE = NewSamplerate;
		m_HostNeedsUpdate = true;
	}
}


void SynthModular::UpdateHostInfo()
{
	/* Pause Audio */
	FreezeAll();

	/* update the settings */
	m_Info.BUFSIZE    = SpiralInfo::BUFSIZE;
	m_Info.SAMPLERATE = SpiralInfo::SAMPLERATE;

	/* obsolete - REMOVE SOON  */
	m_Info.FRAGSIZE   = SpiralInfo::FRAGSIZE;
	m_Info.FRAGCOUNT  = SpiralInfo::FRAGCOUNT;
	m_Info.OUTPUTFILE = SpiralInfo::OUTPUTFILE;
	m_Info.MIDIFILE   = SpiralInfo::MIDIFILE;
	m_Info.POLY       = SpiralInfo::POLY;

	/* Reset all plugin ports/buffers befure Resuming */
	ResetAudio();
}

//////////////////////////////////////////////////////////

// called when a callback output plugin wants to run the audio thread
void SynthModular::cb_Update(void* o, bool mode)
{
	m_CallbackUpdateMode=mode;
	((SynthModular*)o)->Update();
}

// called by a blocking output plugin to notify the engine its ready to
// take control of the update timing (so take the brakes off)
void SynthModular::cb_Blocking(void* o, bool mode)
{
	m_BlockingOutputPluginIsReady=mode;
}

//////////////////////////////////////////////////////////

iostream &SynthModular::StreamPatchIn(iostream &s, bool paste, bool merge)
{
	//if we are merging as opposed to loading a new patch
	//we have no need to pause audio
	if (!merge && !paste)
		FreezeAll();

	//if we are pasting we don't have any of the file version
	//or saving information. since its internal we didn't
	//need it, but we do have other things we might need to load

	bool has_file_path;
	char file_path[1024];
	string m_FromFilePath;

	string dummy,dummy2;		
	int ver;

	if (paste)
	{
		m_Copied.devices>>has_file_path;

		if (has_file_path)
		{
   		  m_Copied.devices.getline(file_path, 1024);
   		  m_FromFilePath = file_path;
   		  cerr << file_path << endl;
   		}  
   		  
	}
	else
	{
		s>>dummy>>dummy>>dummy>>ver;

		if (ver>FILE_VERSION)
		{
			SpiralInfo::Alert("Bad file, or more recent version.");
			ThawAll();
			return s;
		}

		if (ver>2)
		{
			int MainWinX,MainWinY,MainWinW,MainWinH;
			int EditWinX,EditWinY,EditWinW,EditWinH;

			s>>MainWinX>>MainWinY>>MainWinW>>MainWinH;
			s>>EditWinX>>EditWinY>>EditWinW>>EditWinH;

			//o.m_MainWindow->resize(MainWinX,MainWinY,MainWinW,MainWinH);
			//o.m_EditorWindow->resize(EditWinX,EditWinY,EditWinW,EditWinH);
		}
		
		if (merge)
			m_FromFilePath = m_MergeFilePath;
	}

	//wether pasting or merging we need to clear the current 
	//selection so we can replace it with the new devices
	if (paste || merge)
		Fl_Canvas::ClearSelection(m_Canvas);
	
	int Num, ID, PluginID, x,y,ps,px,py;
	
	if (paste)
	{
		Num = m_Copied.devicecount;
	}
	else
	{
		s>>dummy>>Num;
	}

	for(int n=0; n<Num; n++)
	{
		#ifdef DEBUG_STREAM
		cerr<<"Loading Device "<<n<<endl;
		#endif

		s>>dummy; // "Device"
		s>>ID;
		s>>dummy2; // "Plugin"
		s>>PluginID;
		s>>x>>y;

		string Name;

		if (paste || ver>3)
		{
			// load the device name
			int size;
			char Buf[1024];
			s>>size;
			s.ignore(1);
			if (size > 0) {
				s.get(Buf,size+1);
				Name=Buf;
			} else {
				Name = "";
			}
		}

		#ifdef DEBUG_STREAM
		cerr<<dummy<<" "<<ID<<" "<<dummy2<<" "<<PluginID<<" "<<x<<" "<<y<<endl;
		#endif

		if (paste || ver>1) s>>ps>>px>>py;
		
		//if we are merging a patch or pasting we will change duplicate ID's
		if (!paste && !merge)
		{
			// Check we're not duplicating an ID
			if (m_DeviceWinMap.find(ID)!=m_DeviceWinMap.end())
			{
				SpiralInfo::Alert("Duplicate device ID found in file - aborting load");
				ThawAll();
				return s;
			}
		}

		if (PluginID==COMMENT_ID)
		{
			DeviceWin* temp = NewComment(PluginID, x, y);
			if (temp)
			{
				if (paste || merge)
				{
					m_Copied.m_DeviceIds[ID] = m_NextID++;
					ID = m_Copied.m_DeviceIds[ID];
				}	

				temp->m_DeviceGUI->SetID(ID);
				m_DeviceWinMap[ID]=temp;
				((Fl_CommentGUI*)(m_DeviceWinMap[ID]->m_DeviceGUI))->StreamIn(s); // load the plugin

				if (paste || merge)
					Fl_Canvas::AppendSelection(ID, m_Canvas);
				else				
					if (m_NextID<=ID) m_NextID=ID+1;

			}
		}
		else
		{
			DeviceWin* temp = NewDeviceWin(PluginID, x, y);
			if (temp)
			{
				int oldID=ID;
				if (paste || merge)
				{
					m_Copied.m_DeviceIds[ID] = m_NextID++;
	
					ID = m_Copied.m_DeviceIds[ID];
				}

				temp->m_DeviceGUI->SetID(ID);

				if (paste || ver>3)
				{
					// set the titlebars
					temp->m_DeviceGUI->SetName(Name);
				}

				temp->m_Device->SetUpdateInfoCallback(ID,cb_UpdatePluginInfo);
				m_DeviceWinMap[ID]=temp;
				m_DeviceWinMap[ID]->m_Device->StreamIn(s); // load the plugin

				// load external files
				if (paste || merge)
					m_DeviceWinMap[ID]->m_Device->LoadExternalFiles(m_FromFilePath+"_files/", oldID);
				else
					m_DeviceWinMap[ID]->m_Device->LoadExternalFiles(m_FilePath+"_files/");

				if ((paste || ver>1) && m_DeviceWinMap[ID]->m_DeviceGUI->GetPluginWindow())
				{
					// set the GUI up with the loaded values
					// looks messy, but if we do it here, the plugin and it's gui can remain
					// totally seperated.
					((SpiralPluginGUI*)(m_DeviceWinMap[ID]->m_DeviceGUI->GetPluginWindow()))->
						UpdateValues(m_DeviceWinMap[ID]->m_Device);

					// updates the data in the channel buffers, so the values don't
					// get overwritten in the next tick. (should maybe be somewhere else)
					m_DeviceWinMap[ID]->m_Device->GetChannelHandler()->FlushChannels();

					// position the plugin window in the main window
					//m_DeviceWinMap[ID]->m_DeviceGUI->GetPluginWindow()->position(px,py);

					if (ps)
					{
						m_DeviceWinMap[ID]->m_DeviceGUI->Maximise();
						// reposition after maximise
						m_DeviceWinMap[ID]->m_DeviceGUI->position(x,y);
					}
					else m_DeviceWinMap[ID]->m_DeviceGUI->Minimise();
					
					if (paste || merge)
						Fl_Canvas::AppendSelection(ID, m_Canvas);
				}

				if (!paste && !merge)
					if (m_NextID<=ID) m_NextID=ID+1;
			}
			else
			{
				// can't really recover if the plugin ID doesn't match a plugin, as
			    // we have no idea how much data in the stream belongs to this plugin
				SpiralInfo::Alert("Error in stream, can't really recover data from here on.");
				return s;
			}
		}
	}

	if (!paste && !merge)
	{
		s>>*m_Canvas;
		ThawAll();
	}
	
        return s;
}

iostream &operator>>(iostream &s, SynthModular &o)
{
	return o.StreamPatchIn(s, false, false);

}

//////////////////////////////////////////////////////////

ostream &operator<<(ostream &s, SynthModular &o)
{
	o.FreezeAll();

	s<<"SpiralSynthModular File Ver "<<FILE_VERSION<<endl;

	// make external files dir
	bool ExternalDirUsed=false;
	string command("mkdir '"+o.m_FilePath+"_files'");
	system(command.c_str());

	if (FILE_VERSION>2)
	{
		s<<o.m_TopWindow->x()<<" "<<o.m_TopWindow->y()<<" ";
		s<<o.m_TopWindow->w()<<" "<<o.m_TopWindow->h()<<" ";
		s<<0<<" "<<0<<" ";
		s<<0<<" "<<0<<endl;
	}

	// save out the SynthModular
	s<<"SectionList"<<endl;
	s<<o.m_DeviceWinMap.size()<<endl;

	for(map<int,DeviceWin*>::iterator i=o.m_DeviceWinMap.begin();
		i!=o.m_DeviceWinMap.end(); i++)
	{
		if (i->second->m_DeviceGUI && ((i->second->m_Device) || (i->second->m_PluginID==COMMENT_ID)))
		{
			s<<endl;
			s<<"Device ";
			s<<i->first<<" "; // save the id
			s<<"Plugin ";
			s<<i->second->m_PluginID<<endl;
			s<<i->second->m_DeviceGUI->x()<<" ";
			s<<i->second->m_DeviceGUI->y()<<" ";
			s<<i->second->m_DeviceGUI->GetName().size()<<" ";
			s<<i->second->m_DeviceGUI->GetName()<<" ";

			if (i->second->m_DeviceGUI->GetPluginWindow())
			{
				s<<i->second->m_DeviceGUI->GetPluginWindow()->visible()<<" ";
				s<<i->second->m_DeviceGUI->GetPluginWindow()->x()<<" ";
				s<<i->second->m_DeviceGUI->GetPluginWindow()->y()<<" ";
			}
			else
			{
				s<<0<<" "<<0<<" "<<0;
			}

			s<<endl;

			if (i->second->m_PluginID==COMMENT_ID)
			{
				// save the comment gui
				((Fl_CommentGUI*)(i->second->m_DeviceGUI))->StreamOut(s);
			}
			else
			{
				// save the plugin
				i->second->m_Device->StreamOut(s);
			}
			s<<endl;

			// save external files
			if (i->second->m_Device && i->second->m_Device->SaveExternalFiles(o.m_FilePath+"_files/"))
			{
				ExternalDirUsed=true;
			}
		}
	}

	s<<endl<<*o.m_Canvas<<endl;

	// remove it if it wasn't used
	if (!ExternalDirUsed)
	{
		// i guess rmdir won't work if there is something in the dir
		// anyway, but best to be on the safe side. (could do rm -rf) :)
		string command("rmdir "+o.m_FilePath+"_files");
		system(command.c_str());
	}

	o.ThawAll();

	return s;
}

//////////////////////////////////////////////////////////////////////////////////////////

// Callbacks

/////////////////////////////////
// File Menu & associated buttons

// New

inline void SynthModular::cb_New_i (Fl_Widget *o, void *v) {
       if (m_DeviceWinMap.size()>0 && !Pawfal_YesNo ("New - Lose changes to current patch?"))
          return;
       m_TopWindow->label (TITLEBAR.c_str());
       ClearUp();
}

void SynthModular::cb_New (Fl_Widget *o, void *v) {
     ((SynthModular*)(o->user_data()))->cb_New_i (o, v);
}

// Load

inline void SynthModular::cb_Load_i (Fl_Widget *o, void *v) {
       if (m_DeviceWinMap.size()>0 && !Pawfal_YesNo ("Load - Lose changes to current patch?"))
          return;
       char *fn=fl_file_chooser ("Load a patch", "*.ssm", NULL);
       if (fn && fn!='\0') {
          ifstream in (fn);
          if (in) {
             fstream inf;
             inf.open (fn, ios::in);
             m_FilePath = fn;
             ClearUp();
             inf >> *this;
             inf.close();
             TITLEBAR = LABEL + " " + fn;
             m_TopWindow->label (TITLEBAR.c_str());
          }
       }
}

void SynthModular::cb_Load(Fl_Widget *o, void *v) {
     ((SynthModular*)(o->user_data()))->cb_Load_i (o, v);
}

// Save

inline void SynthModular::cb_Save_i (Fl_Widget *o, void *v) {
       char *fn=fl_file_chooser("Save a patch", "*.ssm", NULL);
       if (fn && fn!='\0') {
          ifstream ifl (fn);
          if (ifl) {
             if (!Pawfal_YesNo ("File [%s] exists, overwrite?", fn))
                return;
          }
          ofstream of (fn);
          if (of) {
             m_FilePath = fn;
             of << *this;
             TITLEBAR = LABEL + " " + fn;
             m_TopWindow->label (TITLEBAR.c_str());
          }
          else {
             fl_message (string ("Error saving " + string(fn)).c_str());
          }
       }
}

void SynthModular::cb_Save (Fl_Widget *o, void *v) {
     ((SynthModular*)(o->user_data()))->cb_Save_i (o, v);
}

// Merge

inline void SynthModular::cb_Merge_i (Fl_Widget *o, void *v) {
       char *fn = fl_file_chooser ("Merge a patch", "*.ssm", NULL);
       if (fn && fn!='\0') {
          ifstream in (fn);
          if (in) {
             fstream inf;
             inf.open (fn, ios::in);
             m_MergeFilePath = fn;
             StreamPatchIn (inf, false, true);
             m_Canvas->StreamSelectionWiresIn (inf, m_Copied.m_DeviceIds, true, false);
             inf.close();
          }
       }
}

void SynthModular::cb_Merge (Fl_Widget *o, void *v) {
     ((SynthModular*)(o->parent()->user_data()))->cb_Merge_i (o, v);
}

// Close

inline void SynthModular::cb_Close_i (Fl_Widget *o, void *v) {
       m_SettingsWindow->hide();
       delete m_SettingsWindow;
       m_TopWindow->hide();
       delete m_TopWindow;
}

void SynthModular::cb_Close (Fl_Widget *o, void *v) {
     ((SynthModular*)(o->user_data()))->cb_Close_i (o, v);
}

/////////////////////////////////
// Edit Menu

// Cut

inline void SynthModular::cb_Cut_i(Fl_Widget *o, void *v) {
       if (! m_Canvas->HaveSelection()) return;
       // show some warning here
       cb_Copy_i (o, v);  // should we be calling an inline function here??????
       for (unsigned int i=0; i<m_Canvas->Selection().m_DeviceIds.size(); i++) {
           int ID = m_Canvas->Selection().m_DeviceIds[i];
           Fl_DeviceGUI::Kill(m_DeviceWinMap[ID]->m_DeviceGUI);
       }
       Fl_Canvas::ClearSelection(m_Canvas);
}

void SynthModular::cb_Cut (Fl_Widget *o, void *v) {
     ((SynthModular*)(o->parent()->user_data()))->cb_Cut_i (o, v);
}

// Copy

inline void SynthModular::cb_Copy_i (Fl_Widget *o, void *v) {
       if (! m_Canvas->HaveSelection()) return;
       m_Copied.devices.open ("___temp.ssmcopytmp", ios::out);
       m_Copied.devicecount = 0;
       m_Copied.m_DeviceIds.clear();
       if (m_FilePath != "") {
           m_Copied.devices << true << " "  << m_FilePath << endl;
       }    
       else m_Copied.devices << false << endl;
       for (unsigned int i=0; i<m_Canvas->Selection().m_DeviceIds.size(); i++) {
           int ID = m_Canvas->Selection().m_DeviceIds[i];
           std::map<int,DeviceWin*>::iterator i = m_DeviceWinMap.find(ID);
           m_Copied.m_DeviceIds[ID] = ID;
           m_Copied.devicecount += 1;
           m_Copied.devices << "Device " << i->first << " " ; // save the id
           m_Copied.devices << "Plugin " <<i->second->m_PluginID << endl;
           m_Copied.devices << i->second->m_DeviceGUI->x() << " ";
           m_Copied.devices << i->second->m_DeviceGUI->y() << " ";
           m_Copied.devices << i->second->m_DeviceGUI->GetName().size() << " ";
           m_Copied.devices << i->second->m_DeviceGUI->GetName() << " ";
           if (i->second->m_DeviceGUI->GetPluginWindow()) {
              m_Copied.devices << i->second->m_DeviceGUI->GetPluginWindow()->visible() << " ";
              m_Copied.devices << i->second->m_DeviceGUI->GetPluginWindow()->x() << " ";
              m_Copied.devices << i->second->m_DeviceGUI->GetPluginWindow()->y() << " ";
           }
           else m_Copied.devices << 0 << " " << 0 << " " << 0;
           m_Copied.devices << endl;
           if (i->second->m_PluginID == COMMENT_ID) {
              // save the comment gui
              ((Fl_CommentGUI*)(i->second->m_DeviceGUI))->StreamOut (m_Copied.devices);
           }
           else {
              // save the plugin
              i->second->m_Device->StreamOut (m_Copied.devices);
           }
           m_Copied.devices<<endl;
       }
       m_Canvas->StreamSelectionWiresOut(m_Copied.devices);
       m_Copied.devices.close();
       Fl_Canvas::EnablePaste (m_Canvas);
}

void SynthModular::cb_Copy (Fl_Widget *o, void *v) {
     ((SynthModular*)(o->parent()->user_data()))->cb_Copy_i (o, v);
}

// Paste

inline void SynthModular::cb_Paste_i (Fl_Widget *o, void *v) {
       if (m_Copied.devicecount <= 0) return;
       m_Copied.devices.open ("___temp.ssmcopytmp", ios::in);
       StreamPatchIn(m_Copied.devices, true, false);
       m_Canvas->StreamSelectionWiresIn (m_Copied.devices, m_Copied.m_DeviceIds, false, true);
       m_Copied.devices.close();
}

void SynthModular::cb_Paste (Fl_Widget *o, void *v) {
     ((SynthModular*)(o->parent()->user_data()))->cb_Paste_i (o, v);
}

// Delete

inline void SynthModular::cb_Delete_i (Fl_Widget *o, void *v) {
       m_Canvas->DeleteSelection();
}

void SynthModular::cb_Delete (Fl_Widget *o, void *v) {
     ((SynthModular*)(o->user_data()))->cb_Delete_i (o, v);
}

// Options

inline void SynthModular::cb_Options_i (Fl_Widget *o, void *v) {
       m_SettingsWindow->show();
}

void SynthModular::cb_Options (Fl_Widget* o, void* v) {
     ((SynthModular*)(o->user_data()))->cb_Options_i (o, v);
}

/////////////////////////////////
// Plugin Menu

// This callback has the name that the callback for the canvas menu
// used to have please note - that is now NewDeviceFromCanvasMenu

inline void SynthModular::cb_NewDeviceFromMenu_i (Fl_Widget *o, void *v) {
       AddDevice (*((int*)v));
}

void SynthModular::cb_NewDeviceFromMenu (Fl_Widget *o, void *v) {
     ((SynthModular*)(o->user_data()))->cb_NewDeviceFromMenu_i (o, v);
}

// (Plugin Buttons)

inline void SynthModular::cb_NewDevice_i (Fl_Button *o, void *v) {
       AddDevice (*((int*)v));
}

void SynthModular::cb_NewDevice (Fl_Button *o, void *v) {
     ((SynthModular*)(o->parent()->user_data()))->cb_NewDevice_i (o, v);
}

// (Plugin Canvas Menu)

inline void SynthModular::cb_NewDeviceFromCanvasMenu_i(Fl_Canvas* o, void* v)
{
	AddDevice(*((int*)v),*((int*)v+1),*((int*)v+2));
}
void SynthModular::cb_NewDeviceFromCanvasMenu(Fl_Canvas* o, void* v)
{((SynthModular*)(o->user_data()))->cb_NewDeviceFromCanvasMenu_i(o,v);}


/////////////////////////////////
// Audio Menu

// Play / Pause

inline void SynthModular::cb_PlayPause_i (Fl_Widget *o, void *v) {
       string oldname = m_PlayPause->tooltip ();
       if (m_Info.PAUSED) {
          m_PlayPause->label ("@||");
          m_PlayPause->tooltip ("Pause");
          ResumeAudio();
       }
       else {
          m_PlayPause->label ("@>");
          m_PlayPause->tooltip ("Play");
          PauseAudio();
       }
       for (int i=0; i<m_MainMenu->size(); i++) {
            if (m_MainMenu->text (i) != NULL) {
               if (oldname == m_MainMenu->text (i)) {
                  m_MainMenu->replace (i, m_PlayPause->tooltip());
                  break;
               }
            }
        }

}

void SynthModular::cb_PlayPause (Fl_Widget *o, void *v) {
     ((SynthModular*)(o->user_data()))->cb_PlayPause_i (o, v);
}

// Reset

inline void SynthModular::cb_Reset_i (Fl_Widget *o, void *v) {
       ResetAudio();
}

void SynthModular::cb_Reset (Fl_Widget *o, void *v) {
     ((SynthModular*)(o->user_data()))->cb_Reset_i (o, v);
}

//////////////////////////////////////////////////////////

inline void SynthModular::cb_NewComment_i(Fl_Button* o, void* v)
{
	AddComment(-1);
}
void SynthModular::cb_NewComment(Fl_Button* o, void* v)
{((SynthModular*)(o->parent()->user_data()))->cb_NewComment_i(o,v);}

//////////////////////////////////////////////////////////

inline void SynthModular::cb_GroupTab_i(Fl_Tabs* o, void* v)
{
        m_GroupTab->redraw();
}

void SynthModular::cb_GroupTab(Fl_Tabs* o, void* v)
{((SynthModular*)(o->parent()->user_data()))->cb_GroupTab_i(o,v);}

//////////////////////////////////////////////////////////

inline void SynthModular::cb_Connection_i(Fl_Canvas* o, void* v)
{
	CanvasWire *Wire;
	Wire=(CanvasWire*)v;

	map<int,DeviceWin*>::iterator si=m_DeviceWinMap.find(Wire->OutputID);
	if (si==m_DeviceWinMap.end())
	{
		char num[32];
		sprintf(num,"%d",Wire->OutputID);
		SpiralInfo::Alert("Warning: Connection problem - can't find source "+string(num));
		return;
	}

	map<int,DeviceWin*>::iterator di=m_DeviceWinMap.find(Wire->InputID);
	if (di==m_DeviceWinMap.end())
	{
		char num[32];
		sprintf(num,"%d",Wire->InputID);
		SpiralInfo::Alert("Warning: Connection problem - can't find destination "+string(num));
		return;
	}

	Sample *sample=NULL;

	if (!si->second->m_Device->GetOutput(Wire->OutputPort,&sample))
	{
		char num[32];
		sprintf(num,"%d,%d",Wire->OutputID,Wire->OutputPort);
		SpiralInfo::Alert("Warning: Connection problem - can't find source output "+string(num));
		return;
	}

	if (!di->second->m_Device->SetInput(Wire->InputPort,(const Sample*)sample))
	{
		char num[32];
		sprintf(num,"%d,%d",Wire->InputID,Wire->InputPort);
		SpiralInfo::Alert("Warning: Connection problem - can't find source input "+string(num));
		return;
	}
}
void SynthModular::cb_Connection(Fl_Canvas* o, void* v)
{((SynthModular*)(o->user_data()))->cb_Connection_i(o,v);}

//////////////////////////////////////////////////////////

inline void SynthModular::cb_Unconnect_i(Fl_Canvas* o, void* v)
{
	CanvasWire *Wire;
	Wire=(CanvasWire*)v;

	//cerr<<Wire->InputID<<" "<<Wire->InputPort<<endl;

	map<int,DeviceWin*>::iterator di=m_DeviceWinMap.find(Wire->InputID);
	if (di==m_DeviceWinMap.end())
	{
		//cerr<<"Can't find destination device "<<Wire->InputID<<endl;
		return;
	}

	SpiralPlugin *Plugin=di->second->m_Device;
	if (Plugin && !Plugin->SetInput(Wire->InputPort,NULL))
	{ cerr<<"Can't find destination device's Input"<<endl; return;	}
}
void SynthModular::cb_Unconnect(Fl_Canvas* o, void* v)
{((SynthModular*)(o->user_data()))->cb_Unconnect_i(o,v);}

//////////////////////////////////////////////////////////

void SynthModular::cb_UpdatePluginInfo(int ID, void *PInfo)
{
	map<int,DeviceWin*>::iterator i=m_DeviceWinMap.find(ID);
	if (i!=m_DeviceWinMap.end())
	{
		DeviceGUIInfo Info=BuildDeviceGUIInfo(*((PluginInfo*)PInfo));

		(*i).second->m_DeviceGUI->Setup(Info);
		(*i).second->m_DeviceGUI->redraw();
	}
}

//////////////////////////////////////////////////////////

void SynthModular::LoadPatch(const char *fn)
{
	ifstream in(fn);

	if (in)
	{
		fstream	inf;
		inf.open(fn, std::ios::in);

		m_FilePath=fn;

		ClearUp();
		inf>>*this;

		inf.close();

		TITLEBAR=LABEL+" "+fn;
		m_TopWindow->label(TITLEBAR.c_str());
	}
}

