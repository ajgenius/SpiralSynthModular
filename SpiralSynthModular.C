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

#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Tooltip.H>
#include "SpiralSynthModular.h"
#include "AudioTransportHub.h"
#include "AudioBackend.h"
#include "Midi.h"
#include "DeviceClassRegistry.h"
#include "EditorClassRegistry.h"
#include "OutputPlugin.h"
#include "OutputPluginGUI.h"
#include "SpiralInfo.h"
#include "SpiralPluginGUI.h"
#include "GUI/SSM.xpm"
#include "GUI/load.xpm"
#include "GUI/save.xpm"
#include "GUI/new.xpm"
#include "GUI/options.xpm"
#include "GUI/comment.xpm"
#include "PawfalYesNo.h"
#ifdef __APPLE__
#include "MacBundle.h"
#endif

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

//////////////////////////////////////////////////////////

DeviceWin::~DeviceWin()
{
}

//////////////////////////////////////////////////////////

SynthModular::SynthModular():
m_ResetingAudioThread(false),
m_HostNeedsUpdate(false),
m_Frozen(false),
m_NextID(0)
{
	pthread_mutex_init(&m_CycleLock, NULL);
	/* Shared Audio State Information  */
	m_Info.BUFSIZE = SpiralInfo::BUFSIZE;
	m_Info.SAMPLERATE = SpiralInfo::SAMPLERATE;
	m_Info.PAUSED = false;
	m_Info.FRAME = 0;
	m_Info.ROLLING = true;

	/* obsolete - REMOVE SOON  */
	m_Info.FRAGSIZE = SpiralInfo::FRAGSIZE;
	m_Info.FRAGCOUNT = SpiralInfo::FRAGCOUNT;
	m_Info.OUTPUTFILE = SpiralInfo::OUTPUTFILE;
	m_Info.AUDIOCLIENT = SpiralInfo::AUDIOCLIENT;
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
	AudioTransportHub::Get()->SetHost(&m_Info);
}

//////////////////////////////////////////////////////////

SynthModular::~SynthModular()
{
	// main has stopped the engine; no channel handshake can run now.
	m_Frozen = true;
	ClearUp(false);
	AudioTransportHub::Get()->SetHost(NULL);
	delete m_SettingsWindow;
	delete m_TopWindow;
	// Editors before devices, the reverse of loading.
	spiralcore::EditorClassRegistry::PackUpAndGoHome();
	spiralcore::DeviceClassRegistry::PackUpAndGoHome();
	// The MidiPlugin instances are gone with the devices, so the
	// backend they shared is too; its module can leave now.
	spiralcore::MidiBackendRegistry::PackUpAndGoHome();
	system("rm -f ___temp.ssmcopytmp");
	pthread_mutex_destroy(&m_CycleLock);
}

//////////////////////////////////////////////////////////

void SynthModular::ClearUp(bool synchronize)
{
	if (synchronize) FreezeAll();

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

		// On shutdown, destroy GUI instances while their modules and DSP
		// objects are still present. Widget destructors detach from the canvas.
		if (!synchronize) delete i->second->m_DeviceGUI;
		delete i->second->m_Device;
		i->second->m_Device=NULL;
		if (!synchronize) delete i->second;
	}

	m_Canvas->Clear();
	m_DeviceWinMap.clear();
	m_NextID=0;

	if (synchronize) ThawAll();
}

//////////////////////////////////////////////////////////
// One engine period: wait for the transport to want the next block,
// run control work and the graph under the gate, hand the block over.
// This is the only thread that runs the graph; device callbacks and the
// blocking transport thread only move finished periods.
void SynthModular::Update()
{
	AudioTransportHub *hub=AudioTransportHub::Get();
	if (m_Frozen)
	{
		// Control owns the devices while frozen: only watch for the thaw.
		pthread_mutex_lock(&m_CycleLock);
		m_CH.UpdateDataNow(m_Info.FRAME);
		pthread_mutex_unlock(&m_CycleLock);
		usleep(1000);
		return;
	}

	pthread_mutex_lock(&m_CycleLock);
	m_CH.UpdateDataNow(m_Info.FRAME);
	if (m_Frozen)
	{
		pthread_mutex_unlock(&m_CycleLock);
		return;
	}

	for (map<int,DeviceWin*>::iterator i = m_DeviceWinMap.begin(); i != m_DeviceWinMap.end(); )
	{
		SpiralPlugin *plugin = i->second->m_Device;
		if (plugin && plugin->IsDead())
		{
			delete plugin;
			i->second->m_Device = NULL;
			m_DeviceWinMap.erase(i++);
			continue;
		}

		if (plugin && !m_ResetingAudioThread)
		{
			plugin->UpdateChannelHandler();
			plugin->ExecuteCommands();
			if (plugin->IsAudioDriver())
				static_cast<AudioDriver *>(plugin)->ServiceAudio();

		}

		++i;
	}

	if (m_HostNeedsUpdate)
	{
		m_Info.BUFSIZE = SpiralInfo::BUFSIZE;
		m_Info.SAMPLERATE = SpiralInfo::SAMPLERATE;
		m_ResetingAudioThread = true;
		m_HostNeedsUpdate = false;
	}

	const bool render = hub->PreparePeriod();
	if (render)
	{
		hub->BeginPeriod();
		m_Info.FRAME = hub->Frame();
		m_Info.ROLLING = hub->Rolling();
		RenderAudio();
		hub->CommitPeriod();
	}

	// Only the scalar delay crosses the gate. Control may now detach, destroy
	// or replace any native client while the engine sleeps.
	const unsigned delay = render ? 0 : hub->SleepMicroseconds();
	pthread_mutex_unlock(&m_CycleLock);
	if (delay) usleep(delay);

}

void SynthModular::RenderAudio()
{
	if (!m_ResetingAudioThread)
	{
		for (map<int,DeviceWin*>::iterator i = m_DeviceWinMap.begin(); i != m_DeviceWinMap.end(); ++i)
		{
			SpiralPlugin *plugin = i->second->m_Device;
			if (plugin && !plugin->IsDead() && plugin->IsAudioDriver())
			{
				AudioDriver *driver = static_cast<AudioDriver *>(plugin);
				if (driver->ProcessType() == AudioDriver::ALWAYS)
					driver->ProcessAudio();

			}

		}
	}

	// run the plugins (only ones connected to anything)
	const list<int> &ExecutionOrder = m_Canvas->GetGraph()->GetSortedList();
	for (list<int>::const_reverse_iterator i=ExecutionOrder.rbegin();
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
				di->second->m_Device->StampOutputs(m_Info.FRAME);

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
		 i!=m_DeviceWinMap.end(); )
	{
		if (i->second->m_DeviceGUI && i->second->m_DeviceGUI->GetPluginWindow())
		{
			SpiralPluginGUI *GUI=(SpiralPluginGUI *)i->second->m_DeviceGUI->GetPluginWindow();
			GUI->Update();
		}

		if (i->second->m_DeviceGUI && i->second->m_DeviceGUI->Killed())
		{
			bool erase = true;

			// Audio is walking this graph on another thread. Hold the same
			// gate RenderAudio uses, then drop it before destroying widgets.
			// ~SpiralPluginGUI calls Fl::check(), and Update() will erase this
			// map node as soon as the gate drops, so finish with the iterator first.
			pthread_mutex_lock(&m_CycleLock);

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

			Fl_DeviceGUI *gui = i->second->m_DeviceGUI;
			i->second->m_DeviceGUI = NULL;
			if (erase)
				m_DeviceWinMap.erase(i++);
			else
				++i;

			pthread_mutex_unlock(&m_CycleLock);

			//Delete Device GUI - must delete here or sometimes plugin will randomly crash
			delete gui;
			continue;
		}

		i++;
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
        m_TopWindow->callback(cb_Close, this);
	//m_TopWindow->resizable(m_TopWindow);
        m_MainMenu = new Fl_Menu_Bar (0, 0, MAIN_WIDTH, 20, "");
        m_MainMenu->user_data((void*)(this));
        m_MainMenu->box(FL_PLASTIC_UP_BOX);
        m_MainMenu->textsize (10);
        m_MainMenu->add ("File/New", 0, cb_New, (void*)(this), FL_MENU_DIVIDER);
        m_MainMenu->add ("File/Load", 0, cb_Load, (void*)(this), 0);
        m_MainMenu->add ("File/Save As", 0, cb_Save, (void*)(this), 0);
        m_MainMenu->add ("File/Merge", 0, cb_Merge, (void*)(this), FL_MENU_DIVIDER);
#ifdef __APPLE__
        if (!SSMBundleResourceDirectory("Examples").empty())
            m_MainMenu->add ("File/Examples...", 0, cb_Examples, (void*)(this), FL_MENU_DIVIDER);
#endif
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

// A preference list names modules the way the preferences file always
// has: by file name, with or without the _DSP/_GUI split.
static bool ListedModule(const string &module)
{
	string stem = module.substr(module.rfind('/') + 1);
	stem = stem.substr(0, stem.find('.'));
	string::size_type split = stem.rfind('_');
	if (split != string::npos) stem.erase(split);
	for (vector<string>::const_iterator i = SpiralInfo::PLUGINVEC.begin(); i != SpiralInfo::PLUGINVEC.end(); ++i)
		if (i->compare(0, stem.size(), stem) == 0) return true;
	return false;
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
     string PluginRoot = pluginPath.empty() ? SpiralInfo::PLUGIN_PATH : pluginPath;
     if (!PluginRoot.empty() && PluginRoot[PluginRoot.size()-1] != '/') PluginRoot += '/';
     // Audio backend modules sit beside the device plugins under audio/;
     // compiled-in backends are already registered and a module of the
     // same name yields.
     spiralcore::AudioBackendRegistry::Get()->LoadModules(PluginRoot);
     // MIDI backends under midi/; the preference names one, else the
     // first that is not the dummy. MidiPlugin opens the device itself.
     spiralcore::MidiBackendRegistry::Get()->LoadModules(PluginRoot);
     spiralcore::MidiDevice::SetBackendName(SpiralInfo::MIDIBACKEND);
     // Built-in devices first: a module of the same ID is not loaded.
     spiralcore::DeviceClassRegistry::Get()->Register(OutputPlugin::Class());
     spiralcore::EditorClassRegistry::Get()->Register(OutputPluginGUI::Class());
     // Devices under dsp/, editors under gui/; an editor needs no device
     // module to load, it pairs by ID when a device is made.
     spiralcore::DeviceClassRegistry::Get()->LoadModules(PluginRoot);
     spiralcore::EditorClassRegistry::Get()->LoadModules(PluginRoot);
     const vector<spiralcore::DeviceClass*> &Devices = spiralcore::DeviceClassRegistry::Get()->Classes();
     for (vector<spiralcore::DeviceClass*>::const_iterator i=Devices.begin(); i!=Devices.end(); i++) {
         const spiralcore::DeviceClass *info = *i;
         int ID = info->ID;
         if (SpiralInfo::USEPLUGINLIST && !ListedModule(info->Module)) continue;
         {
            #ifdef DEBUG_PLUGINS
            cerr << ID << " = Plugin [" << info->Module << "]" << endl;
            #endif
            Fl_ToolButton *NewButton = new Fl_ToolButton (0, 0, Width, Height, "");
            // we can't set user data, because the callback uses it
            // NewButton->user_data ((void*)(this));
            NewButton->labelsize (1);
            Fl_Pixmap *tPix = new Fl_Pixmap (info->Icon);
            NewButton->image(tPix->copy(tPix->w(),tPix->h()));
            delete tPix;
            string GroupName = info->Category;
            Fl_Pack* the_group=NULL;
            // find or create this group, and add an icon
            map<string,Fl_Pack*>::iterator gi = m_PluginGroupMap.find (GroupName);
            if (gi == m_PluginGroupMap.end()) {
               the_group = new Fl_Pack (m_GroupTab->x(), 16, m_GroupTab->w(), m_GroupTab->h() - 15, GroupName.c_str());
               the_group->type(FL_HORIZONTAL);

		the_group->copy_label(GroupName.c_str());
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
            the_group->end();

            // we need to keep tooltips stored outside their widgets - widgets just have a pointer
            // I haven't done anything about cleaning up these strings - which may cause memory leaks?
            // But m_DeviceVec - which, I assume, would be used to keep track of / clean up the dynamicly
            //     created NewButton widgets isn't cleaned up either, so we might have 2 memory leaks
            //     involved? - but then again, they might be automatically deallocated because they're
            //     in another widget, in which case there's just one memory leak to deal with. (andy)
            string* PluginName = new string (info->Name);
            string::size_type p;
            NewButton->tooltip (PluginName->c_str());
            // Slashes have significance to the menu widgets, remove them from the GroupName
            while ((p = GroupName.find ('/')) != string::npos)
                  GroupName = GroupName.replace (p, 1, " and ");
            string MenuEntry = "Plugins/" + GroupName + "/" + *PluginName;
            m_MainMenu->add (MenuEntry.c_str(), 0, cb_NewDeviceFromMenu, &Numbers[ID], 0);
            // when help is working better - this will put the plugins into the help menu
            // MenuEntry = "Help/" + MenuEntry;
            // m_MainMenu->add (MenuEntry.c_str(), 0, NULL, &Numbers[ID], 0);

            // Add the plugins to the canvas menu
            m_Canvas->AddPluginName (MenuEntry, info->ID);
            // this overwrites the widget's user_data with that specified for the callback
            // so we can't use it for other purposes
            NewButton->callback ((Fl_Callback*)cb_NewDevice, &Numbers[ID]);
            NewButton->show();
            // Nothing else ever touches m_DeviceVec - is this right??? (andy)
            m_DeviceVec.push_back (NewButton);
            the_group->redraw();
            // m_NextPluginButton++;
            Fl::check();
            splashtext->label (PluginName->c_str());
            Splash->redraw();
         }
     }
     map<string,Fl_Pack*>::iterator PlugGrp;
     for (PlugGrp = m_PluginGroupMap.begin(); PlugGrp!= m_PluginGroupMap.end(); ++PlugGrp) {
         m_GroupTab->add (PlugGrp->second);
         PlugGrp->second->add (new Fl_Box (0, 0, 600, 100, ""));
         PlugGrp->second->end();
     }
     m_GroupTab->end();
     Fl_Group::current(0);
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
	const spiralcore::DeviceClass* Plugin=spiralcore::DeviceClassRegistry::Get()->Find(n);

	if (!Plugin)
	{
		char t[256];
		sprintf(t,"%d",n);
		SpiralInfo::Alert("Plugin "+string(t)+" not found.");
		return NULL;
	}

	nlw->m_Device=Plugin->CreateInstance();

	if (!nlw->m_Device) return NULL;

	nlw->m_Device->SetUpdateCallback(cb_Update);
	nlw->m_Device->SetParent((void*)this);

	if ( nlw->m_Device->IsAudioDriver() )
	{
		AudioDriver *driver = ((AudioDriver*)nlw->m_Device);
		driver->SetChangeBufferAndSampleRateCallback(cb_ChangeBufferAndSampleRate);
	}

	PluginInfo PInfo    = nlw->m_Device->Initialise(&m_Info);
	/* Toolbar Fl_Pack ctors leave Fl_Group::current() on the pack.
	   Constructing the plugin GUI then parents it into the toolbar
	   during the click handler — crash or a window that cannot expand. */
	Fl_Group *prev = Fl_Group::current();
	Fl_Group::current(0);
	// A device with no editor gets a bare device window.
	const spiralcore::EditorClass *Editor=spiralcore::EditorClassRegistry::Get()->Find(n);
	SpiralGUIType *temp = Editor ? Editor->CreateEditor(nlw->m_Device) : NULL;
	if (temp) temp->end();
	Fl_Pixmap *Pix      = new Fl_Pixmap(Plugin->Icon);
	nlw->m_PluginID     = n;

	if (temp) temp->position(x+10,y);

	DeviceGUIInfo Info=BuildDeviceGUIInfo(PInfo);

	Info.XPos       = x; //TOOLBOX_WIDTH+(rand()%400);
	Info.YPos       = y; //rand()%400;

	nlw->m_DeviceGUI = new Fl_DeviceGUI(Info, temp, Pix, nlw->m_Device->IsTerminal());
	nlw->m_DeviceGUI->end();
	Fl_Group::current(prev);
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
	m_Info.AUDIOCLIENT = SpiralInfo::AUDIOCLIENT;
	m_Info.MIDIFILE   = SpiralInfo::MIDIFILE;
	m_Info.POLY       = SpiralInfo::POLY;
	spiralcore::MidiDevice::SetBackendName(SpiralInfo::MIDIBACKEND);

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
					// updates the data in the channel buffers, so the values don't
					// get overwritten in the next tick, and so the GUI can read the
					// loaded state through the channel.
					m_DeviceWinMap[ID]->m_Device->GetChannelHandler()->FlushChannels();

					// set the GUI up with the loaded values
					// looks messy, but if we do it here, the plugin and it's gui can remain
					// totally seperated.
					((SpiralPluginGUI*)(m_DeviceWinMap[ID]->m_DeviceGUI->GetPluginWindow()))->
						UpdateValues(m_DeviceWinMap[ID]->m_Device);

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

spiralcore::Description &Describe(spiralcore::Description &d, SynthModular &o)
{
	o.FreezeAll();

	d.Value("SpiralSynthModular File Ver").Separator(" ").Value(FILE_VERSION).Line();

	// make external files dir
	bool ExternalDirUsed=false;
	string command("mkdir '"+o.m_FilePath+"_files'");
	system(command.c_str());

	if (FILE_VERSION>2)
	{
		d.Value(o.m_TopWindow->x()).Separator(" ").Value(o.m_TopWindow->y()).Separator(" ");
		d.Value(o.m_TopWindow->w()).Separator(" ").Value(o.m_TopWindow->h()).Separator(" ");
		d.Value(0).Separator(" ").Value(0).Separator(" ");
		d.Value(0).Separator(" ").Value(0).Line();
	}

	// save out the SynthModular
	d.Value("SectionList").Line();
	d.Value(o.m_DeviceWinMap.size()).Line();

	for(map<int,DeviceWin*>::iterator i=o.m_DeviceWinMap.begin();
		i!=o.m_DeviceWinMap.end(); i++)
	{
		if (i->second->m_DeviceGUI && ((i->second->m_Device) || (i->second->m_PluginID==COMMENT_ID)))
		{
			d.Line();
			d.Value("Device").Separator(" ");
			d.Value(i->first).Separator(" "); // save the id
			d.Value("Plugin").Separator(" ");
			d.Value(i->second->m_PluginID).Line();
			d.Value(i->second->m_DeviceGUI->x()).Separator(" ");
			d.Value(i->second->m_DeviceGUI->y()).Separator(" ");
			d.Value(i->second->m_DeviceGUI->GetName().size()).Separator(" ");
			d.Value(i->second->m_DeviceGUI->GetName()).Separator(" ");

			if (i->second->m_DeviceGUI->GetPluginWindow())
			{
				d.Value(i->second->m_DeviceGUI->GetPluginWindow()->visible()).Separator(" ");
				d.Value(i->second->m_DeviceGUI->GetPluginWindow()->x()).Separator(" ");
				d.Value(i->second->m_DeviceGUI->GetPluginWindow()->y()).Separator(" ");
			}
			else
			{
				d.Value(0).Separator(" ").Value(0).Separator(" ").Value(0);
			}

			d.Line();

			if (i->second->m_PluginID==COMMENT_ID)
			{
				// save the comment gui
				((Fl_CommentGUI*)(i->second->m_DeviceGUI))->Describe(d);
			}
			else
			{
				// save the plugin
				i->second->m_Device->Describe(d);
			}
			d.Line();

			// save external files
			if (i->second->m_Device && i->second->m_Device->SaveExternalFiles(o.m_FilePath+"_files/"))
			{
				ExternalDirUsed=true;
			}
		}
	}

	d.Line();
	::Describe(d, *o.m_Canvas);
	d.Line();

	// remove it if it wasn't used
	if (!ExternalDirUsed)
	{
		// i guess rmdir won't work if there is something in the dir
		// anyway, but best to be on the safe side. (could do rm -rf) :)
		string command("rmdir "+o.m_FilePath+"_files");
		system(command.c_str());
	}

	o.ThawAll();

	return d;
}

ostream &operator<<(ostream &s, SynthModular &o)
{
	spiralcore::Description d;
	Describe(d, o);
	d.Write(s);
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

void SynthModular::ChooseAndLoadPatch(const char *directory) {
       if (m_DeviceWinMap.size()>0 && !Pawfal_YesNo ("Load - Lose changes to current patch?"))
          return;
       char *fn=fl_file_chooser (directory ? "Load an example patch" : "Load a patch", "*.ssm", directory);
       if (fn && *fn!='\0') {
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

inline void SynthModular::cb_Load_i (Fl_Widget *o, void *v) {
     ChooseAndLoadPatch(NULL);
}

#ifdef __APPLE__
void SynthModular::cb_Examples(Fl_Widget *o, void *v) {
     std::string directory = SSMBundleResourceDirectory("Examples");
     if (directory.empty()) {
          fl_message("The bundled Examples folder is unavailable.");
          return;
     }
     directory += "/";
     ((SynthModular*)v)->ChooseAndLoadPatch(directory.c_str());
}
#endif

void SynthModular::cb_Load(Fl_Widget *o, void *v) {
     ((SynthModular*)(o->user_data()))->cb_Load_i (o, v);
}

// Save

inline void SynthModular::cb_Save_i (Fl_Widget *o, void *v) {
       char *fn=fl_file_chooser("Save a patch", "*.ssm", NULL);
       if (fn && *fn!='\0') {
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
              fl_message ( "%s", string ("Error saving " + string(fn)).c_str());
          }
       }
}

void SynthModular::cb_Save (Fl_Widget *o, void *v) {
     ((SynthModular*)(o->user_data()))->cb_Save_i (o, v);
}

// Merge

inline void SynthModular::cb_Merge_i (Fl_Widget *o, void *v) {
       char *fn = fl_file_chooser ("Merge a patch", "*.ssm", NULL);
       if (fn && *fn!='\0') {
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
       // Leave widgets alive until engine and plugin cleanup has finished.
       m_SettingsWindow->hide();
       m_TopWindow->hide();
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
           std::map<int,DeviceWin*>::iterator found = m_DeviceWinMap.find(ID);
           if (found == m_DeviceWinMap.end() || !found->second || !found->second->m_DeviceGUI)
              continue;
           Fl_DeviceGUI::Kill(found->second->m_DeviceGUI);
       }
       Fl_Canvas::ClearSelection(m_Canvas);
}

void SynthModular::cb_Cut (Fl_Widget *o, void *v) {
     if (!v) return;
     ((SynthModular*)v)->cb_Cut_i (o, v);
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
           std::map<int,DeviceWin*>::iterator j = m_DeviceWinMap.find(ID);
           if (j == m_DeviceWinMap.end() || !j->second || !j->second->m_DeviceGUI)
              continue;
           if (j->second->m_PluginID != COMMENT_ID && !j->second->m_Device)
              continue;
           m_Copied.m_DeviceIds[ID] = ID;
           m_Copied.devicecount += 1;
           m_Copied.devices << "Device " << j->first << " " ; // save the id
           m_Copied.devices << "Plugin " <<j->second->m_PluginID << endl;
           m_Copied.devices << j->second->m_DeviceGUI->x() << " ";
           m_Copied.devices << j->second->m_DeviceGUI->y() << " ";
           m_Copied.devices << j->second->m_DeviceGUI->GetName().size() << " ";
           m_Copied.devices << j->second->m_DeviceGUI->GetName() << " ";
           if (j->second->m_DeviceGUI->GetPluginWindow()) {
              m_Copied.devices << j->second->m_DeviceGUI->GetPluginWindow()->visible() << " ";
              m_Copied.devices << j->second->m_DeviceGUI->GetPluginWindow()->x() << " ";
              m_Copied.devices << j->second->m_DeviceGUI->GetPluginWindow()->y() << " ";
           }
           else m_Copied.devices << 0 << " " << 0 << " " << 0;
           m_Copied.devices << endl;
           if (j->second->m_PluginID == COMMENT_ID) {
              // save the comment gui
              ((Fl_CommentGUI*)(j->second->m_DeviceGUI))->StreamOut (m_Copied.devices);
           }
           else {
              // save the plugin
              j->second->m_Device->StreamOut (m_Copied.devices);
           }
           m_Copied.devices<<endl;
       }
       m_Canvas->StreamSelectionWiresOut(m_Copied.devices);
       m_Copied.devices.close();
       Fl_Canvas::EnablePaste (m_Canvas);
}

void SynthModular::cb_Copy (Fl_Widget *o, void *v) {
     if (!v) return;
     ((SynthModular*)v)->cb_Copy_i (o, v);
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
     if (!v) return;
     ((SynthModular*)v)->cb_Paste_i (o, v);
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

inline void SynthModular::cb_NewDeviceFromCanvasMenu_i (Fl_Canvas* o, void* v) {
       AddDevice(*((int*)v),*((int*)v+1),*((int*)v+2));
}

void SynthModular::cb_NewDeviceFromCanvasMenu(Fl_Canvas* o, void* v) {
     ((SynthModular*)(o->user_data()))->cb_NewDeviceFromCanvasMenu_i(o,v);
}


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

		bool preserve = dynamic_cast<StablePortLayout*>((*i).second->m_Device) != NULL;
		(*i).second->m_DeviceGUI->SetupPorts(Info, false, preserve);
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

