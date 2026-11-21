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
#include <memory>
#include <set>
#include <cctype>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Tooltip.H>
#include "SpiralSynthModular.h"
#include "AudioTransportHub.h"
#include "AudioBackend.h"
#include "BackendPath.h"
#include "Midi.h"
#include "DeviceClassRegistry.h"
#include "PanelRegistry.h"
#include "PanelWindow.h"
#include "libs/libSpiralPanels/include/Panel.h"
#include "PanelControl.h"
#include "FltkView.h"
#include "OutputPlugin.h"
#include "EventInputPlugin.h"
#include "SubPatchDevice.h"
#ifdef HAVE_LADSPA
#include "LADSPAPlugin.h"
#endif
#include "PatchProject.h"
#include "PatchFile.h"
#include <memory>
#include <FL/fl_ask.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Window.H>
#include "UnavailablePlugin.h"
#include "SpiralInfo.h"
#include "SpiralPluginGUI.h"
#include "JSON.h"
#include "PositionalReader.h"
#include <iterator>
#include "GUI/SSM.xpm"
#include "GUI/SpiralIcon.xpm"
#include "GUI/load.xpm"
#include "GUI/save.xpm"
#include "GUI/new.xpm"
#include "GUI/options.xpm"
#include "GUI/comment.xpm"
#include "PawfalYesNo.h"
#include "MacBundle.h"
#ifndef SSM_EXAMPLES_DIR
#define SSM_EXAMPLES_DIR ""
#endif

//#define DEBUG_PLUGINS
//#define DEBUG_STREAM

const static string LABEL = "SpiralSynthModular "+VER_STRING;
static string TITLEBAR;

static const int FILE_VERSION = 4;

static string ContractDirectory()
{
	const char *override = getenv("SSM_SCHEMA_DIR");

	if (override)
		return override;

	string bundle = SpiralBundleResourceDirectory("schemas");

	if (!bundle.empty())
		return bundle;

	const string installed = SSM_INSTALLED_SCHEMA_DIR;

	if (access((installed + "/SpiralPositionalText-0.2.x.json").c_str(), R_OK) == 0
	    && access((installed + "/SpiralPositionalText.history.json").c_str(), R_OK) == 0)
		return installed;

	return SPIRALCORE_SCHEMA_DIR;
}

static Spiral::Core::PositionalReader *ContractReader()
{
	static bool tried = false;
	static std::auto_ptr<Spumoni::JSON> contract;
	static std::auto_ptr<Spumoni::JSON> history;
	static std::auto_ptr<Spiral::Core::PositionalReader> reader;

	if (!tried)
	{
		tried = true;
		string dir = ContractDirectory();
		string error;

		contract.reset(Spumoni::ParseJSON((dir + "/SpiralPositionalText-0.2.x.json").c_str(), &error));
		if (contract.get())
			history.reset(Spumoni::ParseJSON((dir + "/SpiralPositionalText.history.json").c_str(), &error));

		if (contract.get() && history.get())
			reader.reset(new Spiral::Core::PositionalReader(*contract, history.get()));
		else
			cerr << "SpiralSynthModular: no file contract in " << dir << ": " << error << endl;
	}

	return reader.get();
}

static int LoadAs(int PluginID)
{
	Spiral::Core::PositionalReader *reader = ContractReader();

	return reader ? (int)reader->LoadAs(PluginID) : PluginID;
}

static bool ApplyDeviceState(iostream &s, const string &text, int PluginID, SpiralPlugin *plugin)
{
	Spiral::Core::PositionalReader *reader = ContractReader();

	if (reader)
	{
		Spiral::Core::Description state;
		size_t at = s.tellg(), consumed = 0;
		string error;

		if (reader->ReadState(PluginID, text, at, state, consumed, error))
		{
			plugin->Migrate(state);
			Spiral::Core::Description::Reader values(state);
			plugin->Apply(values);
			s.seekg(at + consumed);

			return true;
		}

		char id[32];
		sprintf(id, "%d", PluginID);
		SpiralInfo::Alert("Plugin "+string(id)+" state is not under the file contract ("+error+") - aborting load");

		return false;
	}

	SpiralInfo::Alert("No file contract - cannot load a patch");

	return false;
}

static std::string JSONDocument(std::istream &s)
{
	std::streampos here = s.tellg();
	int c = s.peek();

	while (c == ' ' || c == '\t' || c == '\r' || c == '\n')
	{
		s.get();
		c = s.peek();
	}

	if (c != '{')
	{
		s.clear();
		s.seekg(here);

		return "";
	}

	std::string text((std::istreambuf_iterator<char>(s)), std::istreambuf_iterator<char>());

	return text;
}

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
m_NextID(0),
m_RunVoices(false),
m_VoiceTopology(0),
m_MainMenu(NULL),
m_SysMenu(NULL),
m_WantChrome(false),
m_Topbar(NULL),
m_Canvas(NULL),
m_CanvasScroll(NULL),
m_Drawer(NULL),
m_Breadcrumb(NULL),
m_TopWindow(NULL),
m_PatchVoices(0),
m_VoicesStale(false),
m_SavePointCount(0),
m_MenuStateKnown(false),
m_MenuSelection(false),
m_MenuPaste(false),
m_MenuSavePoints(false),
m_MenuPackage(false),
m_MidiLearn(NULL),
m_MidiLearnData(NULL),
m_MidiPage(NULL),
m_PickDevice(-1),
m_PickOpened(false)
{
	pthread_mutexattr_t recursive;

	pthread_mutexattr_init(&recursive);
	pthread_mutexattr_settype(&recursive, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_CycleLock, &recursive);
	pthread_mutexattr_destroy(&recursive);
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

	for (int n=0; n<512; n++)
		Numbers[n]=n;

	m_CH.Register("Frozen",&m_Frozen);
	AudioTransportHub::Get()->SetHost(&m_Info);
}

//////////////////////////////////////////////////////////

SynthModular::~SynthModular()
{
	m_Frozen = true;
	ClearUp(false);
	AudioTransportHub::Get()->SetHost(NULL);
	delete m_SettingsWindow;
	delete m_TopWindow;

	Spiral::Core::PanelRegistry::PackUpAndGoHome();
	Spiral::Core::DeviceClassRegistry::PackUpAndGoHome();
	Spiral::Core::MidiDevice::PackUpAndGoHome();
	Spiral::Core::MidiBackendRegistry::PackUpAndGoHome();
	pthread_mutex_destroy(&m_CycleLock);
}

//////////////////////////////////////////////////////////

void SynthModular::StopAudioForExit()
{
	// Stop the native consumer while the graph can still supply periods.
	// Teardown of the roll and its voices can outlast the queued audio.
	pthread_mutex_lock(&m_CycleLock);
	AudioTransportHub *hub = AudioTransportHub::Get();
	if (hub->GetMode() != AudioTransportHub::NO_MODE && hub->GetMode() != AudioTransportHub::CLOSED)
		hub->Close();
	pthread_mutex_unlock(&m_CycleLock);
	FreezeAll();
}

void SynthModular::ClearUp(bool synchronize)
{
	if (synchronize)
		FreezeAll();

	ClearVoices();

	m_Canvas->Clear();

	for(map<int,DeviceWin*>::iterator i=m_DeviceWinMap.begin(); i!=m_DeviceWinMap.end(); i++)
	{
		//Stop processing of audio if any
		if (i->second->m_Device)
		{
			i->second->m_Device->Kill();
		}

		i->second->m_DeviceGUI->Clear();

		if (i->second->m_DeviceGUI->GetPluginWindow())
		{
			i->second->m_DeviceGUI->GetPluginWindow()->hide();
		}

		if (!synchronize)
			delete i->second->m_DeviceGUI;

		delete i->second->m_Device;
		i->second->m_Device=NULL;

		if (!synchronize)
			delete i->second;
	}

	m_Canvas->ResetView();
	m_DeviceWinMap.clear();
	m_NextID=0;
	m_PatchVoices=0;

	if (synchronize)
		SetPatchVoices(0);

	StopMidiPick();
	m_MidiMap.Devices.clear();
	m_MidiLearn=NULL;

	if (m_MidiPage)
		m_MidiPage->Refresh();

	if (synchronize)
		ThawAll();
}

//////////////////////////////////////////////////////////

void SynthModular::Update()
{
	AudioTransportHub *hub=AudioTransportHub::Get();

	if (m_Frozen)
	{
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

	if (Spiral::Core::MidiDevice *midi = Spiral::Core::MidiDevice::Get())
		midi->Collect();

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

	const unsigned delay = render ? 0 : hub->SleepMicroseconds();
	pthread_mutex_unlock(&m_CycleLock);

	if (delay)
		usleep(delay);
}

void SynthModular::RenderAudio()
{
	m_RunVoices = m_Canvas->Topology() == m_VoiceTopology;

	if (!m_ResetingAudioThread)
	{
		for (
				map<int,DeviceWin*>::iterator i = m_DeviceWinMap.begin();
				i != m_DeviceWinMap.end();
				++i
			)
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
	const list<int> &ExecutionOrder = m_Canvas->ExecutionOrder();

	for (list<int>::const_reverse_iterator i=ExecutionOrder.rbegin(); i!=ExecutionOrder.rend(); i++)
		// use the graphsort order to remove internal latency
		RunDevice(*i);

	//we can safely turn this off here
	m_ResetingAudioThread = false;
}

void SynthModular::cb_RunDevice(int id, void *host)
{
	((SynthModular *)host)->RunDevice(id);
}

void SynthModular::RunDevice(int id)
{
	map<int,DeviceWin*>::iterator di=m_DeviceWinMap.find(id);

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
			di->second->m_Device->PublishFeeds();

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

		map<int, VoicePool *>::iterator pool = m_VoicePools.find(id);

		if (pool != m_VoicePools.end())
			pool->second->Run(id, di->second->m_Device, m_RunVoices, m_ResetingAudioThread, m_Info.FRAME);

		#ifdef DEBUG_PLUGINS
		cerr<<"Finished executing"<<endl;
		#endif
	}
}

//////////////////////////////////////////////////////////

void SynthModular::UpdatePluginGUIs()
{
	ServiceMidiInput();

	if (m_Breadcrumb)
	{
		string root = m_FilePath;
		const size_t slash = root.find_last_of("/\\");
		if (slash != string::npos)
			root = root.substr(slash + 1);

		const size_t dot = root.find_last_of('.');
		if (dot != string::npos && dot > 0)
			root = root.substr(0, dot);

		m_Breadcrumb->SetRoot(root);
		m_Breadcrumb->Refresh();
	}

	SyncSubPatches();

	if (m_VoicesStale || m_Canvas->Topology() != m_VoiceTopology)
		RebuildVoices();

	SyncVoices();

	// see if any need deleting
	for (map<int,DeviceWin*>::iterator i=m_DeviceWinMap.begin(); i!=m_DeviceWinMap.end(); )
	{
		if (i->second->m_DeviceGUI && i->second->m_DeviceGUI->GetPluginWindow())
		{
			SpiralGUIType *window = i->second->m_DeviceGUI->GetPluginWindow();
			if (SpiralPluginGUI *GUI = dynamic_cast<SpiralPluginGUI *>(window))
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
				i->second->m_Device->Kill();
				erase = false;
			}

			//Clear GUI Device
			i->second->m_DeviceGUI->Clear();

			// Hide Device GUI FIRST
			if (i->second->m_DeviceGUI->GetPluginWindow())
			{
				i->second->m_DeviceGUI->GetPluginWindow()->hide();
			}

			if (m_Canvas->IsBox(i->first))
				ClearVoices();

			if (m_Canvas->IsBox(i->first))
				for (
						map<int,DeviceWin*>::iterator m = m_DeviceWinMap.begin();
						m != m_DeviceWinMap.end();
						++m
					)

					if (m->second->m_DeviceGUI && m_Canvas->PageOf(m->first) == i->first)
						Fl_DeviceGUI::Kill(m->second->m_DeviceGUI);

			//Remove Device GUI from canvas
			m_Canvas->RemoveDevice(i->second->m_DeviceGUI);

			char listed[32];

			snprintf(listed, sizeof listed, "%d", i->first);

			if (m_PickDevice == i->first)
				StopMidiPick();

			if (m_MidiMap.Remove(listed) && m_MidiPage)
				m_MidiPage->Refresh();

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

Spiral::Panels::PanelControl *SynthModular::MidiControl(const string &device, const string &control)
{
	char *end = NULL;
	const int id = (int)strtol(device.c_str(), &end, 10);
	map<int,DeviceWin*>::iterator i = m_DeviceWinMap.find(id);

	if (device.empty() || *end || i == m_DeviceWinMap.end() || !i->second->m_DeviceGUI || control.empty())
		return NULL;

	Spiral::Panels::PanelWindow *window = dynamic_cast<Spiral::Panels::PanelWindow *>(i->second->m_DeviceGUI->GetPluginWindow());
	Spiral::Panels::Panel *panel = window ? window->GetPanel() : NULL;

	return panel ? panel->FindControl(control) : NULL;
}

void SynthModular::LearnMidi(void (*learned)(int, int, void *), void *data)
{
	m_MidiLearn = learned;
	m_MidiLearnData = data;
}

namespace
{
	struct MidiPanelMove
	{
		Spiral::Core::MidiMap::Target Target;
		int Value;
	};
}

void SynthModular::ServiceMidiInput()
{
	Spiral::Core::MidiDevice *midi = Spiral::Core::MidiDevice::Get();
	if (!midi)
		return;

	vector<MidiPanelMove> moves;
	vector<Spiral::Core::MidiMap::Target> targets;
	map<pair<string, string>, size_t> continuous;
	Spiral::Core::MidiPacket packet;
	int channel, controller, value;

	while (midi->ReadPatchInput(packet))
	{
		if (!Spiral::Core::ControlChange(packet, channel, controller, value))
			continue;

		if (m_MidiLearn)
		{
			void (*learned)(int, int, void *) = m_MidiLearn;
			m_MidiLearn = NULL;
			learned(channel, controller, m_MidiLearnData);

			continue;
		}

		targets.clear();
		m_MidiMap.Match(channel, controller, targets);
		for (size_t t = 0; t < targets.size(); ++t)
		{
			const Spiral::Core::MidiMap::Target &target = targets[t];
			Spiral::Panels::PanelControl *control = MidiControl(target.Device, target.Control);
			if (!control)
				continue;

			if (dynamic_cast<Fl_Button *>(Spiral::Panels::FltkView::WidgetOf(*control)))
			{
				// Keep edges in order, including the knob positions at each press.
				continuous.clear();
			}
			else
			{
				const pair<string, string> key(target.Device, target.Control);
				map<pair<string, string>, size_t>::iterator previous = continuous.find(key);
				if (previous != continuous.end())
					moves[previous->second].Value = -1;
				continuous[key] = moves.size();
			}

			MidiPanelMove move = {target, value};
			moves.push_back(move);
		}
	}

	for (size_t i = 0; i < moves.size(); ++i)
	{
		const MidiPanelMove &move = moves[i];
		if (move.Value < 0)
			continue;
		if (Spiral::Panels::PanelControl *control = MidiControl(move.Target.Device, move.Target.Control))
			control->Drive(move.Value / 127.0);
	}
}

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
	m_TopWindow->add (m_MainMenu);

#ifdef __APPLE__
	if (m_WantChrome)
	{
		m_SysMenu = new Fl_Sys_Menu_Bar (0, 0, MAIN_WIDTH, 1, "");
		m_SysMenu->user_data((void*)(this));
	}
#endif

	MenuAdd ("File/New", cb_New, (void*)(this), FL_MENU_DIVIDER);
	MenuAdd ("File/Load", cb_Load, (void*)(this), 0);
	MenuAdd ("File/Save", cb_Save, (void*)(this), 0);
	MenuAdd ("File/Save As", cb_SaveAs, (void*)(this), FL_MENU_DIVIDER);
	MenuAdd ("File/Merge", cb_Merge, (void*)(this), 0);
	MenuAdd ("File/Merge as Subpatch", cb_MergeAsSubPatch, (void*)(this), 0);
	MenuAdd ("File/Import as Branch", cb_ImportBranch, (void*)(this), FL_MENU_DIVIDER);
	MenuAdd ("File/Save Points", cb_SavePoints, (void*)(this), 0);
	MenuAdd ("File/Examples", cb_Examples, (void*)(this), FL_MENU_DIVIDER);
	MenuAdd ("File/Exit", cb_Close, (void*)(this), 0);
	MenuAdd ("Edit/Cut", cb_Cut, (void*)(this), 0);
	MenuAdd ("Edit/Copy", cb_Copy, (void*)(this), 0);
	MenuAdd ("Edit/Paste", cb_Paste, (void*)(this), 0);
	MenuAdd ("Edit/Paste as Subpatch", cb_PasteAsSubPatch, (void*)(this), 0);
	MenuAdd ("Edit/Delete", cb_Delete, (void*)(this), FL_MENU_DIVIDER);
	//MenuAdd ("Edit/Toolbars/Plugins", cb_Undefined, (void*)(this), 0);
	//MenuAdd ("Edit/Toolbars/Function", cb_Undefined, (void*)(this), 0);
	MenuAdd ("Edit/Options", cb_Options, (void*)(this), 0);
	MenuAdd ("View/Center Patch", cb_CenterPatch, (void*)(this), 0);
	MenuAdd ("View/MIDI Bindings", cb_MidiBindings, (void*)(this), 0);
	MenuAdd ("Plugins/dummy", NULL, NULL, 0);
	MenuAdd ("Audio/Pause", cb_PlayPause, NULL, 0);
	MenuAdd ("Audio/Reset", cb_Reset, NULL, 0);
	//MenuAdd ("Help/Plugins/dummy", NULL, NULL, 0);
	//MenuAdd ("Help/Credits", NULL, (void*)(this), 0);

#ifdef __APPLE__
	Fl_Sys_Menu_Bar::about (cb_About, (void*)(this));
#else
	MenuAdd ("Help/About", cb_About, (void*)(this), 0);
#endif

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

	m_Topbar->resizable(NULL);

	/////////////////

	ToolbarHeight += 20; // Stretch this a bit to allow room for the menu-bar too.
	m_CanvasScroll = new Fl_Scroll (0, ToolbarHeight, MAIN_WIDTH, MAIN_HEIGHT-ToolbarHeight, "");
	m_TopWindow->add(m_CanvasScroll);
	m_TopWindow->resizable(m_CanvasScroll);

	m_Canvas = new Fl_Canvas(-5000, -5000, 10000, 10000, "");
	m_Canvas->SetGraphLock(&m_CycleLock);
	m_PatchPool.SetLookup(cb_FindDevice, this);
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
	m_Canvas->SetPasteAsSubPatchCallback((Fl_Callback*)cb_PasteAsSubPatch);
	m_Canvas->SetMergePatchCallback((Fl_Callback*)cb_Merge);
	m_Canvas->SetAboutPatchCallback((Fl_Callback*)cb_AboutPatch);
	m_Canvas->SetGroupCallback((Fl_Callback*)cb_Group);
	m_Canvas->SetUngroupCallback((Fl_Callback*)cb_Ungroup);
	m_Canvas->SetPageCallback((Fl_Callback*)cb_PageShown);
	m_Canvas->SetDropFileCallback((Fl_Callback*)cb_DropFile);

	m_CanvasScroll->add(m_Canvas);

	m_Breadcrumb = new Fl_Breadcrumb (0, MAIN_HEIGHT - Fl_Breadcrumb::Height(), MAIN_WIDTH, Fl_Breadcrumb::Height());
	m_Breadcrumb->color(SpiralInfo::GUICOL_Tool);
	m_Breadcrumb->Bind(m_Canvas);
	m_TopWindow->add(m_Breadcrumb);

	m_Drawer = new Fl_BoundaryDrawer (MAIN_WIDTH, ToolbarHeight, 1, MAIN_HEIGHT-ToolbarHeight);
	m_Drawer->color(SpiralInfo::GUICOL_Tool);
	m_Drawer->selection_color(SpiralInfo::GUICOL_Button);
	m_Drawer->CollapseChanged = cb_DrawerCollapse;
	m_Drawer->AddEvents = cb_AddEvents;
	m_Drawer->AddEventsData = this;
	m_Drawer->PatchVoicesChanged = cb_PatchVoices;
	m_Drawer->PatchVoicesChangedData = this;
	m_Drawer->CollapseChangedData = this;
	m_Drawer->Bind(&m_Document);
	m_Drawer->BindPorts(m_Canvas);
	m_TopWindow->add(m_Drawer);

	m_MidiPage = new Fl_MidiBindings (0, 0, 1, 1);
	m_MidiPage->color(SpiralInfo::GUICOL_Tool);
	m_MidiPage->DeviceName = cb_MidiDeviceName;
	m_MidiPage->PickDevice = cb_PickMidiDevice;
	m_MidiPage->PickControl = cb_PickMidiControl;
	m_MidiPage->Learn = cb_LearnMidi;
	m_MidiPage->SourcePicked = cb_MidiSource;
	m_MidiPage->Data = this;
	m_MidiPage->Bind(&m_MidiMap);
	m_Drawer->BindMidi(m_MidiPage);

	m_SettingsWindow = new SettingsWindow;
	m_SettingsWindow->RegisterApp(this);

	m_TopWindow->Relayout = cb_Relayout;
	m_TopWindow->RelayoutData = this;

	ApplyViewOptions();

	return m_TopWindow;
}

//////////////////////////////////////////////////////////

static bool ListedModule(const string &module)
{
	string stem = module.substr(module.rfind('/') + 1);
	stem = stem.substr(0, stem.find('.'));
	string::size_type split = stem.rfind('_');

	if (split != string::npos)
		stem.erase(split);

	for (
			vector<string>::const_iterator i = SpiralInfo::PLUGINVEC.begin();
			i != SpiralInfo::PLUGINVEC.end();
			++i
		)
		if (i->compare(0, stem.size(), stem) == 0)
			return true;

	return false;
}

void SynthModular::LoadPlugins (string pluginPath)
{
	int Width  = 35;
	int Height = 35;
	int SWidth  = 256;
	int SHeight = 256;
	Fl_Pixmap pic (SSM_xpm);
	Fl_Double_Window* Splash = NULL;
	Fl_Box *splashtext = NULL;

	if (m_WantChrome)
	{
		Splash = new Fl_Double_Window ((Fl::w()/2) - (SWidth/2), (Fl::h()/2) - (SHeight/2),
		                               SWidth, SHeight, "SSM");
		Splash->border(0);
		Fl_Box* pbut = new Fl_Box (0, 8, SWidth, SHeight, "");
		pbut->box (FL_NO_BOX);
		pic.label (pbut);
		splashtext = new Fl_Box (5, SHeight-20, 200, 20, "Loading...");
		splashtext->labelsize (10);
		splashtext->box (FL_NO_BOX);
		splashtext->align (FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
		Fl_Box *splashversion = new Fl_Box (SWidth-115, SHeight-22, 110, 20, VERSION " GIT");
		splashversion->labelsize (12);
		splashversion->box (FL_NO_BOX);
		splashversion->align (FL_ALIGN_INSIDE | FL_ALIGN_RIGHT);
		Splash->add (pbut);
		Splash->add (splashtext);
		Splash->add (splashversion);
		Splash->show();
	}

	string PluginRoot = pluginPath.empty() ? SpiralInfo::PLUGIN_PATH : pluginPath;
	if (!PluginRoot.empty() && PluginRoot[PluginRoot.size()-1] != '/')
		PluginRoot += '/';

	Spiral::Core::LoadAudioBackends(PluginRoot);
	Spiral::Core::LoadMidiBackends(PluginRoot);
	Spiral::Core::MidiDevice::SetBackendName(SpiralInfo::MIDIBACKEND);
	Spiral::Core::MidiDevice::SetDeviceName(SpiralInfo::MIDIFILE);
	Spiral::Core::MidiDevice::Init("SpiralModular", Spiral::Core::MidiDevice::READ);
	Spiral::Panels::PanelWindow::Provide();
	Spiral::Core::DeviceClassRegistry::Get()->LoadModules(PluginRoot);
	const vector<Spiral::Core::DeviceClass*> &Devices = Spiral::Core::DeviceClassRegistry::Get()->Classes();

	for (
	     vector<Spiral::Core::DeviceClass*>::const_iterator i=Devices.begin();
	     i!=Devices.end();
	     i++
	     )
	{
		const Spiral::Core::DeviceClass *info = *i;
		int ID = info->ID;
		if (ID == SubPatchDevice::Class().ID)
			continue;

		if (SpiralInfo::USEPLUGINLIST && !ListedModule(info->Module.empty() ? info->Directory() : info->Module))
			continue;

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

			if (gi == m_PluginGroupMap.end())
			{
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
			else
				the_group = gi->second;

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
			MenuAdd (MenuEntry, cb_NewDeviceFromMenu, &Numbers[ID], 0);
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

			if (splashtext)
				splashtext->label (PluginName->c_str());

			if (Splash)
				Splash->redraw();
		}
	}

	map<string,Fl_Pack*>::iterator PlugGrp;

	for (PlugGrp = m_PluginGroupMap.begin(); PlugGrp!= m_PluginGroupMap.end(); ++PlugGrp)
	{
		m_GroupTab->add (PlugGrp->second);
		PlugGrp->second->add (new Fl_Box (0, 0, 600, 100, ""));
		PlugGrp->second->end();
	}

	m_GroupTab->end();
	Fl_Group::current(0);
	// try to show the SpiralSound group
	PlugGrp = m_PluginGroupMap.find("SpiralSound");
	// can't find it - show the first plugin group
	if (PlugGrp==m_PluginGroupMap.end())
		PlugGrp=m_PluginGroupMap.begin();

	m_GroupTab->value(PlugGrp->second);
	MenuDropPlaceholders();
	m_Canvas->AddAboutPatch();

	if (Splash)
	{
		Splash->hide();
		delete Splash;
	}
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

static string SavedDeviceName(DeviceWin *win)
{
	if (win && win->m_Device)
	{
		UnavailablePlugin *missing = dynamic_cast<UnavailablePlugin*>(win->m_Device);

		if (missing)
			return missing->SavedName();
	}

	if (win && win->m_DeviceGUI)
		return win->m_DeviceGUI->GetName();

	return "";
}

static int RemapLoadedId(bool remap, map<int,int> &ids, int id)
{
	if (!remap)
		return id;

	map<int,int>::iterator found = ids.find(id);

	if (found == ids.end())
		return id;

	return found->second;
}

static void NoteUnavailablePort(map<int,DeviceWin*> &devs, int id, bool input, int port)
{
	if (port < 0)
		return;

	map<int,DeviceWin*>::iterator found = devs.find(id);

	if (found == devs.end() || !found->second->m_Device)
		return;

	found->second->m_Device->RestoreUnavailablePorts(input ? port + 1 : 0, input ? 0 : port + 1);
}

static void GrowUnavailablePorts(
		iostream &s,
		bool merge,
		map<int,DeviceWin*> &devs,
		map<int,int> &remap
	)
{
	streampos mark = s.tellg();

	if (mark < streampos(0))
		return;

	int num = 0;
	s >> num;
	if (!s)
	{
		s.clear();
		s.seekg(mark);

		return;
	}

	bool eight = num == -1;

	if (eight)
	{
		int version = 0;
		s >> version >> num;
	}

	if (num < 0)
		num = 0;

	for (int n = 0; n < num; n++)
	{
		int outId, inId, outPort, inPort, dummy;
		int outTerm = 0, inTerm = 0;

		s >> outId >> dummy >> outPort;

		if (eight)
			s >> outTerm;

		s >> inId >> dummy >> inPort;

		if (eight)
			s >> inTerm;

		if (!s)
			break;

		outId = RemapLoadedId(merge, remap, outId);
		inId = RemapLoadedId(merge, remap, inId);
		NoteUnavailablePort(devs, outId, false, outPort);
		NoteUnavailablePort(devs, inId, true, inPort);
	}

	s.clear();
	s.seekg(mark);
}

static const int POSHSAMPLER_ID = 32;

static int OnlySamplerFiles(const string &dir, int id)
{
	set<unsigned> groups;

	if (DIR *directory = opendir(dir.c_str()))
	{
		while (struct dirent *entry = readdir(directory))
		{
			unsigned group, slot;
			int length = 0;

			if (sscanf(entry->d_name, "PoshSampler%u_%u.wav%n", &group, &slot, &length) == 2
			    && length == (int)strlen(entry->d_name) && slot < 8)
				groups.insert(group);
		}

		closedir(directory);
	}

	if (groups.size() != 1 || groups.count(id))
		return -1;

	return *groups.begin();
}

DeviceWin* SynthModular::NewDeviceWin(int n, int x, int y)
{
	DeviceWin *nlw = new DeviceWin;
	const Spiral::Core::DeviceClass* Plugin=Spiral::Core::DeviceClassRegistry::Get()->Find(n);
	bool missing = false;

	nlw->m_Device = NULL;

	if (Plugin)
		nlw->m_Device=Plugin->CreateInstance();

	if (!nlw->m_Device)
	{
		missing = true;
		nlw->m_Device = new UnavailablePlugin(n);
	}

	nlw->m_Device->SetUpdateCallback(cb_Update);
	nlw->m_Device->SetParent((void*)this);

	if ( nlw->m_Device->IsAudioDriver() )
	{
		AudioDriver *driver = ((AudioDriver*)nlw->m_Device);
		driver->SetChangeBufferAndSampleRateCallback(cb_ChangeBufferAndSampleRate);
	}

	PluginInfo PInfo    = nlw->m_Device->Initialise(&m_Info);
	Fl_Group *prev = Fl_Group::current();
	Fl_Group::current(0);
	// A device with no panel gets a bare device window; a missing plugin
	// gets no panel and no icon.
	const Spiral::Core::PanelClass *Class = missing ? NULL : Spiral::Core::PanelRegistry::Get()->Find(n);

	SpiralGUIType *temp = Class ? Class->Open(nlw->m_Device) : NULL;

	if (temp)
		temp->end();

	Fl_Pixmap *Pix      = (!missing && Plugin) ? new Fl_Pixmap(Plugin->Icon) : NULL;
	nlw->m_PluginID     = n;

	if (temp)
		temp->position(x+10,y);

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
		m_Canvas->SetPageOf(ID, m_Canvas->Page());
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

	Fl_Group *prev = Fl_Group::current();
	nlw->m_DeviceGUI = new Fl_CommentGUI(Info, NULL, NULL);
	nlw->m_DeviceGUI->end();
	Fl_Group::current(prev);

	Fl_Canvas::SetDeviceCallbacks(nlw->m_DeviceGUI, m_Canvas);
	m_Canvas->add(nlw->m_DeviceGUI);
	m_Canvas->redraw();

	return nlw;
}

//////////////////////////////////////////////////////////

DeviceWin* SynthModular::NewSubPatch(
		int x,
		int y,
		const string &name,
		SubPatchDevice *device,
		const Spiral::Core::SubPatchConfig &config
	)
{
	DeviceWin *nlw = new DeviceWin;
	nlw->m_Device = device;
	nlw->m_PluginID = SubPatchDevice::Class().ID;
	device->SetUpdateCallback(cb_Update);
	device->SetParent((void*)this);
	device->SetMemberStep(cb_RunDevice, this);
	device->Voices().SetLookup(cb_FindDevice, this);
	device->Initialise(&m_Info);
	device->SetConfig(config);
	device->PortsChanged();

	const PluginInfo &edge = device->GetPluginInfo();
	DeviceGUIInfo Info;

	Info.XPos = x;
	Info.YPos = y;
	Info.NumInputs = edge.NumInputs;
	Info.NumOutputs = edge.NumOutputs;
	Info.PortTips = edge.PortTips;
	Info.PortTypes = edge.PortTypes;
	const int ports = Info.NumInputs > Info.NumOutputs ? Info.NumInputs : Info.NumOutputs;
	Info.Width = 80;
	Info.Height = 20 + ports * 12;
	Info.Name = name;

	Fl_Group *prev = Fl_Group::current();
	Fl_Group::current(NULL);
	const Spiral::Core::PanelClass *Class = Spiral::Core::PanelRegistry::Get()->Find(nlw->m_PluginID);
	SpiralGUIType *panel = Class ? Class->Open(device) : NULL;
	if (panel)
		panel->end();

	Fl_Group::current(prev);

	if (panel)
		panel->position(x + 10, y);

	nlw->m_DeviceGUI = new Fl_DeviceGUI(Info, panel, NULL);
	nlw->m_DeviceGUI->end();
	Fl_Group::current(prev);
	nlw->m_DeviceGUI->SetName(name);
	Fl_Canvas::SetDeviceCallbacks(nlw->m_DeviceGUI, m_Canvas);
	m_Canvas->add(nlw->m_DeviceGUI);
	m_Canvas->redraw();

	return nlw;
}

static bool HostIO(int plugin)
{
	return plugin == 0 || plugin == 2 || plugin == 31 || plugin == 40;
}

static int EdgePort(
		Fl_Canvas *canvas,
		vector<BoundaryPort> &ports,
		bool input,
		int holder,
		int port
	)
{
	for (size_t k = 0; k < ports.size(); ++k)
		if (ports[k].InnerID == holder && ports[k].InnerPort == port)
			return (int)k;

	const string label = canvas->PortName(holder, input, port);
	string name = label;

	for (unsigned suffix = 2; !name.empty(); ++suffix)
	{
		bool taken = false;

		for (size_t k = 0; k < ports.size() && !taken; ++k)
			taken = ports[k].Name == name;

		if (!taken)
			break;

		char number[16];

		snprintf(number, sizeof number, " %u", suffix);
		name = label + number;
	}

	ports.push_back(BoundaryPort(holder, port, name,
		canvas->IsEventPort(holder, input, port) ? "MusicalEventPort" : "AudioPort"));
	return (int)ports.size() - 1;
}

void SynthModular::GroupSelection(const string &title)
{
	const CanvasGroup selection = m_Canvas->Selection();
	const int page = m_Canvas->Page();
	std::set<int> members;
	int left = 0, top = 0, right = 0, bottom = 0;

	for (size_t s = 0; s < selection.m_DeviceIds.size(); ++s)
	{
		map<int,DeviceWin*>::iterator i = m_DeviceWinMap.find(selection.m_DeviceIds[s]);

		if (i == m_DeviceWinMap.end() || !i->second->m_DeviceGUI)
			continue;

		if (HostIO(i->second->m_PluginID))
			continue;

		Fl_DeviceGUI *gui = i->second->m_DeviceGUI;
		if (members.empty() || gui->x() < left)
			left = gui->x();

		if (members.empty() || gui->y() < top)
			top = gui->y();

		if (members.empty() || gui->x() + gui->w() > right)
			right = gui->x() + gui->w();

		if (members.empty() || gui->y() + gui->h() > bottom)
			bottom = gui->y() + gui->h();

		members.insert(i->first);
	}

	if (members.empty())
		return;

	SubPatchDevice *box = new SubPatchDevice;
	vector<CanvasWire> cut;
	const vector<CanvasWire> &wires = m_Canvas->Wires();

	for (size_t w = 0; w < wires.size(); ++w)
	{
		const int source = m_Canvas->Holder(wires[w].OutputID, page);
		const int dest = m_Canvas->Holder(wires[w].InputID, page);
		const bool fromInside = members.count(source) != 0, toInside = members.count(dest) != 0;
		if (fromInside == toInside)
			continue;

		if (fromInside && m_Canvas->IsEventPort(wires[w].OutputID, false, wires[w].OutputPort))
		{
			cut.push_back(wires[w]);

			continue;
		}

		const int holder = fromInside ? source : dest;
		const int port = fromInside ? m_Canvas->PortThrough(source, false, wires[w].OutputID, wires[w].OutputPort)
		                            : m_Canvas->PortThrough(dest, true, wires[w].InputID, wires[w].InputPort);

		if (port < 0)
			continue;

		EdgePort(m_Canvas, fromInside ? box->Outputs : box->Inputs, !fromInside, holder, port);
	}

	vector<pair<BoundaryPort *, int> > rebound;
	SubPatchDevice *outer = m_Canvas->Box(page);

	for (int input = 1; outer && input >= 0; --input)
	{
		vector<BoundaryPort> &edge = input ? outer->Inputs : outer->Outputs;

		for (size_t k = 0; k < edge.size(); ++k)
			if (members.count(edge[k].InnerID))
				rebound.push_back(make_pair(&edge[k], EdgePort(m_Canvas, input ? box->Inputs : box->Outputs,
					input != 0, edge[k].InnerID, edge[k].InnerPort)));
	}

	for (size_t w = 0; w < cut.size(); ++w)
		m_Canvas->EraseWire(cut[w]);

	const int ID = m_NextID++;
	DeviceWin *win = NewSubPatch((left + right) / 2 - 50, (top + bottom) / 2 - 20, title, box,
		Spiral::Core::SubPatchConfig());
	win->m_DeviceGUI->SetID(ID);
	m_DeviceWinMap[ID] = win;
	m_Canvas->SetBox(ID, box);
	m_Canvas->SetPageOf(ID, page);

	for (std::set<int>::iterator m = members.begin(); m != members.end(); ++m)
		m_Canvas->SetPageOf(*m, ID);

	for (size_t r = 0; r < rebound.size(); ++r)
	{
		rebound[r].first->InnerID = ID;
		rebound[r].first->InnerPort = rebound[r].second;
	}

	m_Canvas->ShowPage(page);
}

void SynthModular::UngroupSelection()
{
	const CanvasGroup selection = m_Canvas->Selection();
	if (selection.m_DeviceIds.size() != 1)
		return;

	{
		const int box = selection.m_DeviceIds[0];
		map<int,DeviceWin*>::iterator win = m_DeviceWinMap.find(box);
		const SubPatchDevice *ungrouped = m_Canvas->Box(box);
		if (win == m_DeviceWinMap.end() || !ungrouped)
			return;

		const int page = m_Canvas->PageOf(box);

		for (
				map<int,DeviceWin*>::iterator m = m_DeviceWinMap.begin();
				m != m_DeviceWinMap.end();
				++m
			)

			if (m_Canvas->PageOf(m->first) == box)
				m_Canvas->SetPageOf(m->first, page);

		for (
				map<int,DeviceWin*>::iterator q = m_DeviceWinMap.begin();
				q != m_DeviceWinMap.end();
				++q
			)
		{
			SubPatchDevice *outer = m_Canvas->Box(q->first);
			if (!outer || q->first == box)
				continue;

			for (int input = 1; input >= 0; --input)
			{
				vector<BoundaryPort> &ports = input ? outer->Inputs : outer->Outputs;
				const vector<BoundaryPort> &through = input ? ungrouped->Inputs : ungrouped->Outputs;

				for (size_t k = 0; k < ports.size(); ++k)
					if (ports[k].InnerID == box && ports[k].InnerPort >= 0 && ports[k].InnerPort < (int)through.size())
					{
						ports[k].InnerID = through[ports[k].InnerPort].InnerID;
						ports[k].InnerPort = through[ports[k].InnerPort].InnerPort;
					}
			}
		}

		pthread_mutex_lock(&m_CycleLock);
		ClearVoices();
		pthread_mutex_unlock(&m_CycleLock);
		m_Canvas->RemoveBox(box);
		Fl_DeviceGUI::Kill(win->second->m_DeviceGUI);
	}
	m_Canvas->ShowPage(m_Canvas->Page());
}

void SynthModular::AddEventInput()
{
	const int box = m_Canvas->Page();
	SubPatchDevice *edited = m_Canvas->Box(box);
	if (!edited)
		return;

	string name = "Events";

	for (unsigned suffix = 2; ; ++suffix)
	{
		bool taken = false;

		for (size_t k = 0; k < edited->Inputs.size() && !taken; ++k)
			taken = edited->Inputs[k].Name == name;

		if (!taken)
			break;

		char number[16];

		snprintf(number, sizeof number, "Events %u", suffix);
		name = number;
	}

	const int before = m_NextID;
	AddDevice(289);

	if (m_NextID == before)
		return;

	m_Canvas->AddBoxPort(box, true, name, "MusicalEventPort");
	m_Canvas->BindBoxPort(box, true, (int)m_Canvas->Box(box)->Inputs.size() - 1, m_NextID - 1, 0);
}

void SynthModular::SyncSubPatches()
{
	bool changed = false;

	for (map<int,DeviceWin*>::iterator i = m_DeviceWinMap.begin(); i != m_DeviceWinMap.end(); ++i)
	{
		SubPatchDevice *device = m_Canvas->Box(i->first);
		if (!device)
			continue;

		const Spiral::Core::SubPatchConfig was = device->Config();

		if (!device->TakeSettings())
			continue;

		if (device->Config().Mode != was.Mode)
			m_Canvas->SetBoxMode(i->first, device->Config().Mode);

		if (device->Config().Voices != was.Voices)
			m_Canvas->SetBoxVoices(i->first, device->Config().Voices);

		changed = true;
	}

	if (changed && m_Drawer)
		m_Drawer->Refresh();
}

VoicePool *SynthModular::PoolFor(int id, long &voices)
{
	voices = 1;
	map<int,DeviceWin*>::iterator i = m_DeviceWinMap.find(id);

	if (i == m_DeviceWinMap.end() || !i->second->m_Device || i->second->m_Device->IsAudioDriver()
	    || dynamic_cast<VoiceSumInput *>(i->second->m_Device)
	    || (HostIO(i->second->m_PluginID) && !dynamic_cast<VoiceOutputSource *>(i->second->m_Device))
	    || m_Canvas->IsBox(id))
		return NULL;

	for (
			int page = m_Canvas->PageOf(id), depth = 0;
			page != -1 && depth < 64;
			page = m_Canvas->PageOf(page), ++depth
		)
	{
		SubPatchDevice *box = m_Canvas->Box(page);
		if (box && m_Canvas->Owns(page) && box->Config().Voices > 0)
		{
			voices = box->Config().Voices;

			return voices > 1 ? &box->Voices() : NULL;
		}
	}

	voices = m_PatchVoices ? m_PatchVoices : 1;

	return voices > 1 ? &m_PatchPool : NULL;
}

SpiralPlugin *SynthModular::cb_FindDevice(int id, void *)
{
	map<int,DeviceWin*>::iterator i = m_DeviceWinMap.find(id);

	return i == m_DeviceWinMap.end() ? NULL : i->second->m_Device;
}

void SynthModular::ClearVoices(bool keepVoiceOutputs)
{
	std::set<VoicePool *> pools;

	for (map<int, VoicePool *>::iterator p = m_VoicePools.begin(); p != m_VoicePools.end(); ++p)
		pools.insert(p->second);

	for (std::set<VoicePool *>::iterator p = pools.begin(); p != pools.end(); ++p)
		(*p)->Clear(keepVoiceOutputs);

	m_VoicePools.clear();
}

void SynthModular::SetPatchVoices(unsigned voices)
{
	m_PatchVoices = voices;
	m_VoicesStale = true;

	if (m_Drawer)
		m_Drawer->SetPatchVoices(voices, voices ? voices : 1);
}

void SynthModular::cb_PatchVoices(unsigned voices, void *data)
{
	((SynthModular *)data)->SetPatchVoices(voices);
}

void SynthModular::RebuildVoices()
{
	m_VoicesStale = false;
	bool pooled = !m_VoicePools.empty();
	long voices = 1;

	for (
			map<int,DeviceWin*>::iterator i = m_DeviceWinMap.begin();
			i != m_DeviceWinMap.end() && !pooled;
			++i
		)
		pooled = PoolFor(i->first, voices) != NULL;

	if (!pooled)
	{
		m_VoiceTopology = m_Canvas->Topology();

		return;
	}

	FreezeAll();
	ClearVoices(true);

	for (map<int,DeviceWin*>::iterator i = m_DeviceWinMap.begin(); i != m_DeviceWinMap.end(); ++i)
	{
		VoicePool *pool = PoolFor(i->first, voices);
		if (!pool)
		{
			SpiralPlugin *device = i->second->m_Device;
			if (dynamic_cast<VoiceOutputSource *>(device))
				device->SetVoice(device, 0, 1);
			continue;
		}
		const Spiral::Core::DeviceClass *Plugin = Spiral::Core::DeviceClassRegistry::Get()->Find(i->second->m_PluginID);

		if (!Plugin)
			continue;

		pool->Add(i->first, i->second->m_Device, *Plugin, (unsigned)voices, &m_Info, m_Sidecars);
		m_VoicePools[i->first] = pool;
	}

	const vector<CanvasWire> &wires = m_Canvas->Wires();

	for (int pass = 0; pass < 2; ++pass)
		for (size_t w = 0; w < wires.size(); ++w)
		{
			map<int, VoicePool *>::iterator source = m_VoicePools.find(wires[w].OutputID);
			map<int, VoicePool *>::iterator dest = m_VoicePools.find(wires[w].InputID);
			VoicePool *from = source == m_VoicePools.end() ? NULL : source->second;
			VoicePool *into = dest == m_VoicePools.end() ? NULL : dest->second;
			const VoicePool::Wire wire = {wires[w].OutputID, wires[w].OutputPort, wires[w].InputID, wires[w].InputPort};

			if (pass == 0 && from && from != into)
				from->SumOut(wire, m_Info.BUFSIZE);
			else if (pass == 1 && into)
				into->Feed(wire, from == into);
		}

	m_VoiceTopology = m_Canvas->Topology();
	ThawAll();
}

void SynthModular::SyncVoices()
{
	if (m_VoicePools.empty() || m_Canvas->Topology() != m_VoiceTopology)
		return;

	std::set<VoicePool *> pools;

	for (map<int, VoicePool *>::iterator p = m_VoicePools.begin(); p != m_VoicePools.end(); ++p)
		pools.insert(p->second);

	bool rebuild = false;
	pthread_mutex_lock(&m_CycleLock);

	for (std::set<VoicePool *>::iterator p = pools.begin(); p != pools.end() && !rebuild; ++p)
		rebuild = !(*p)->Follow();

	pthread_mutex_unlock(&m_CycleLock);

	if (rebuild)
		RebuildVoices();
}

void SynthModular::cb_Group(Fl_Widget *o, void *v)
{
	SynthModular *synth = (SynthModular *)v;
	const CanvasGroup selection = synth->m_Canvas->Selection();
	size_t blocked = 0;

	for (size_t s = 0; s < selection.m_DeviceIds.size(); ++s)
	{
		map<int,DeviceWin*>::iterator i = synth->m_DeviceWinMap.find(selection.m_DeviceIds[s]);

		if (i != synth->m_DeviceWinMap.end() && HostIO(i->second->m_PluginID))
			++blocked;
	}

	if (blocked && blocked == selection.m_DeviceIds.size())
	{
		fl_alert("You can't group Jack, Output, MIDI or Keyboard into a SubPatch.");

		return;
	}

	if (blocked && fl_choice("You can't group Jack, Output, MIDI or Keyboard into a SubPatch.\n"
		"Continue without them?", "Cancel", "Continue", NULL) != 1)

		return;

	size_t leaving = 0;
	const int page = synth->m_Canvas->Page();
	std::set<int> members;

	for (size_t s = 0; s < selection.m_DeviceIds.size(); ++s)
	{
		map<int,DeviceWin*>::iterator i = synth->m_DeviceWinMap.find(selection.m_DeviceIds[s]);

		if (i != synth->m_DeviceWinMap.end() && !HostIO(i->second->m_PluginID))
			members.insert(i->first);
	}

	const vector<CanvasWire> &wires = synth->m_Canvas->Wires();

	for (size_t w = 0; w < wires.size(); ++w)
		if (members.count(synth->m_Canvas->Holder(wires[w].OutputID, page))
		    && !members.count(synth->m_Canvas->Holder(wires[w].InputID, page))
		    && synth->m_Canvas->IsEventPort(wires[w].OutputID, false, wires[w].OutputPort))
			++leaving;

	if (leaving && fl_choice("Events can't leave a SubPatch.\n"
		"Group and cut the event wires leaving it?", "Cancel", "Group", NULL) != 1)

		return;

	const char *title = fl_input("Group name:", "Group");
	if (title)
		synth->GroupSelection(title);
}

void SynthModular::cb_Ungroup(Fl_Widget *o, void *v)
{
	((SynthModular *)v)->UngroupSelection();
}

void SynthModular::cb_PageShown(Fl_Widget *o, void *v)
{
	SynthModular *synth = (SynthModular *)v;
	if (synth->m_Drawer)
		synth->m_Drawer->Refresh();
}

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
		m_Canvas->SetPageOf(ID, m_Canvas->Page());
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
	Spiral::Core::MidiDevice::SetDeviceName(SpiralInfo::MIDIFILE);
	Spiral::Core::MidiDevice::SetBackendName(SpiralInfo::MIDIBACKEND);

	/* Reset all plugin ports/buffers befure Resuming */
	ResetAudio();
}

//////////////////////////////////////////////////////////

void SynthModular::ApplyViewOptions()
{
	if (m_MainMenu)
	{
		if (SpiralInfo::SHOWMENUBAR)
			m_MainMenu->show();
		else
			m_MainMenu->hide();
	}

	LayoutChrome();

	if (m_Canvas)
		m_Canvas->redraw();
}

void SynthModular::MenuAdd(const string &path, Fl_Callback *cb, void *data, int flags)
{
	if (m_MainMenu)
		m_MainMenu->add(path.c_str(), 0, cb, data, flags);

	if (m_SysMenu)
		m_SysMenu->add(path.c_str(), 0, cb, data, flags);
}

template<class Bar> static void RemovePlaceholders(Bar *bar)
{
	bool found;
	int item;

	do
	{
		found = false;

		for (item = 0; item < bar->size(); item++)
		{
			if (bar->text (item) != NULL)
			{
				found = (strcmp ("dummy", bar->text (item)) == 0);
				if (found)
					break;
			}
		}

		if (found)
			bar->remove (item);
	} while (found);
}

template<class Bar> static void RelabelItem(
		Bar *bar,
		const string &oldLabel,
		const string &newLabel
	)
{
	for (int item = 0; item < bar->size(); item++)
	{
		if (bar->text (item) != NULL && oldLabel == bar->text (item))
		{
			bar->replace (item, newLabel.c_str());

			break;
		}
	}
}

template<class Bar> static void EnableItem(Bar *bar, const string &label, bool on)
{
	for (int item = 0; item < bar->size(); item++)
	{
		if (bar->text (item) != NULL && label == bar->text (item))
		{
			const int flags = bar->mode (item);
			bar->mode (item, on ? (flags & ~FL_MENU_INACTIVE)
			                    : (flags | FL_MENU_INACTIVE));

			break;
		}
	}
}

void SynthModular::MenuEnable(const string &label, bool on)
{
	if (m_MainMenu)
		EnableItem(m_MainMenu, label, on);

	if (m_SysMenu)
		EnableItem(m_SysMenu, label, on);
}

void SynthModular::RefreshMenuState()
{
	if (!m_Canvas)
		return;

	const bool selection = m_Canvas->HaveSelection();
	const bool paste = m_Canvas->CanPaste();
	const bool points = m_SavePointCount > 0;
	const bool package = Spiral::File::Project::PathLooksLikePackage(m_FilePath);

	if (m_MenuStateKnown && selection == m_MenuSelection &&
	    paste == m_MenuPaste && points == m_MenuSavePoints &&
	    package == m_MenuPackage)
		return;

	m_MenuStateKnown = true;
	m_MenuSelection = selection;
	m_MenuPaste = paste;
	m_MenuSavePoints = points;
	m_MenuPackage = package;

	MenuEnable("Cut", selection);
	MenuEnable("Copy", selection);
	MenuEnable("Delete", selection);
	MenuEnable("Paste", paste);
	MenuEnable("Paste as Subpatch", paste);
	MenuEnable("Save Points", points);
	MenuEnable("Import as Branch", package);
}

void SynthModular::MenuDropPlaceholders()
{
	if (m_MainMenu)
		RemovePlaceholders(m_MainMenu);

	if (m_SysMenu)
		RemovePlaceholders(m_SysMenu);
}

void SynthModular::MenuRelabel(const string &oldLabel, const string &newLabel)
{
	if (m_MainMenu)
		RelabelItem(m_MainMenu, oldLabel, newLabel);

	if (m_SysMenu)
		RelabelItem(m_SysMenu, oldLabel, newLabel);
}

void SynthModular::LayoutChrome()
{
	if (!m_MainMenu || !m_Topbar || !m_CanvasScroll || !m_TopWindow)
		return;

	const int MenuHeight = m_MainMenu->visible() ? m_MainMenu->h() : 0;
	const int ChromeHeight = MenuHeight + m_Topbar->h();

	m_MainMenu->resize(0, 0, m_TopWindow->w(), m_MainMenu->h());
	m_Topbar->resize(0, MenuHeight, m_TopWindow->w(), m_Topbar->h());

	if (m_GroupTab)
	{
		int Fixed = 0;

		for (int i = 0; i < m_Topbar->children(); ++i)
			if (m_Topbar->child(i) != m_GroupTab)
				Fixed += m_Topbar->child(i)->w();

		const int TabWidth = m_TopWindow->w() - Fixed;

		if (TabWidth > 0)
			m_GroupTab->size(TabWidth, m_GroupTab->h());
	}

	const int DrawerWidth = m_Drawer ? m_Drawer->CurrentWidth() : 0;
	const int CrumbHeight = m_Breadcrumb ? m_Breadcrumb->h() : 0;

	if (m_Drawer)
		m_Drawer->resize(m_TopWindow->w() - DrawerWidth, ChromeHeight,
		                 DrawerWidth, m_TopWindow->h() - ChromeHeight);

	m_CanvasScroll->resize(0, ChromeHeight, m_TopWindow->w() - DrawerWidth,
	                       m_TopWindow->h() - ChromeHeight - CrumbHeight);

	if (m_Breadcrumb)
		m_Breadcrumb->resize(0, m_TopWindow->h() - CrumbHeight, m_TopWindow->w() - DrawerWidth, CrumbHeight);

	m_TopWindow->redraw();
}

// called when a callback output plugin wants to run the audio thread
void SynthModular::cb_Update(void* o, bool mode)
{
	m_CallbackUpdateMode=mode;
	((SynthModular*)o)->Update();
}

//////////////////////////////////////////////////////////

iostream &SynthModular::StreamPatchIn(iostream &s, bool merge, const string &sidecars)
{
	FreezeAll();

	string m_FromFilePath;
	string dummy,dummy2;
	int ver;

	string text;

	{
		streampos here = s.tellg();
		text.assign(istreambuf_iterator<char>(s), istreambuf_iterator<char>());
		s.clear();
		s.seekg(here);
	}

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

	if (merge)
		Fl_Canvas::ClearSelection(m_Canvas);

	int Num;
	vector<pair<int, int> > samplers;

	s>>dummy>>Num;

	for(int n=0; n<Num; n++)
	{
		#ifdef DEBUG_STREAM
		cerr<<"Loading Device "<<n<<endl;
		#endif

		int ID, PluginID, x, y;
		int ps = 0, px = 0, py = 0;

		// Some files count more devices than they hold, where the original
		// left one out as it saved. What follows the last device is then the
		// wires, and read as a device it gave "Duplicate device ID" and the
		// patch came in with no wires at all. The devices end where the
		// next word is not "Device".
		const std::streampos here = s.tellg();

		s>>dummy; // "Device"

		if (dummy != "Device")
		{
			cerr << "SSM: The file counts " << Num << " devices and holds " << n << ".\n";
			s.clear();
			s.seekg(here);

			break;
		}

		s>>ID;
		s>>dummy2; // "Plugin"
		s>>PluginID;
		s>>x>>y;

		string Name;

		if (ver>3)
		{
			// load the device name
			int size;
			char Buf[1024];

			s>>size;
			s.ignore(1);

			if (size > 0)
			{
				s.get(Buf,size+1);
				Name=Buf;
			}
			else
			{
				Name = "";
			}
		}

		#ifdef DEBUG_STREAM
		cerr<<dummy<<" "<<ID<<" "<<dummy2<<" "<<PluginID<<" "<<x<<" "<<y<<endl;
		#endif

		if (ver>1)
			s>>ps>>px>>py;

		//if we are merging a patch or pasting we will change duplicate ID's
		if (!merge)
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
				if (merge)
				{
					m_Copied.m_DeviceIds[ID] = m_NextID++;
					ID = m_Copied.m_DeviceIds[ID];
				}

				temp->m_DeviceGUI->SetID(ID);
				m_DeviceWinMap[ID]=temp;
				((Fl_CommentGUI*)(m_DeviceWinMap[ID]->m_DeviceGUI))->StreamIn(s); // load the plugin

				if (merge)
					Fl_Canvas::AppendSelection(ID, m_Canvas);
				else if (m_NextID<=ID)
					m_NextID=ID+1;
			}
		}
		else
		{
			DeviceWin* temp = NewDeviceWin(LoadAs(PluginID), x, y);
			if (temp)
			{
				int oldID=ID;

				if (merge)
				{
					m_Copied.m_DeviceIds[ID] = m_NextID++;

					ID = m_Copied.m_DeviceIds[ID];
				}

				temp->m_DeviceGUI->SetID(ID);

				if (ver>3)
				{
					// set the titlebars
					temp->m_DeviceGUI->SetName(Name);
				}

				if (UnavailablePlugin *missingDevice = dynamic_cast<UnavailablePlugin*>(temp->m_Device))
				{
					missingDevice->SetSavedName(Name);
					string label = Name.empty() ? string("Plugin") : Name;
					temp->m_DeviceGUI->SetName(label + " (unavailable)");
					cerr << "SSM: Missing plugin " << label << " (" << PluginID
						<< "); preserving its state and wires.\n";
				}

				temp->m_Device->SetUpdateInfoCallback(ID,cb_UpdatePluginInfo);
				m_DeviceWinMap[ID]=temp;

				if (!ApplyDeviceState(s, text, PluginID, m_DeviceWinMap[ID]->m_Device)) // load the plugin
				{
					ThawAll();

					return s;
				}

				// load external files
				if (!sidecars.empty())
					m_DeviceWinMap[ID]->m_Device->LoadExternalFiles(sidecars, merge ? oldID : -1);
				else if (merge)
					m_DeviceWinMap[ID]->m_Device->LoadExternalFiles(m_FromFilePath+"_files/", oldID);
				else
					m_DeviceWinMap[ID]->m_Device->LoadExternalFiles(m_FilePath+"_files/");

				if (LoadAs(PluginID) == POSHSAMPLER_ID)
					samplers.push_back(make_pair(ID, merge ? oldID : ID));

				m_DeviceWinMap[ID]->m_Device->GetChannelHandler()->FlushChannels();

				if (ver>1 && m_DeviceWinMap[ID]->m_DeviceGUI->GetPluginWindow())
				{
					// set the GUI up with the loaded values
					// looks messy, but if we do it here, the plugin and it's gui can remain
					// totally seperated.
					if (SpiralPluginGUI *gui = dynamic_cast<SpiralPluginGUI *>(m_DeviceWinMap[ID]->m_DeviceGUI->GetPluginWindow()))
						gui->UpdateValues(m_DeviceWinMap[ID]->m_Device);

					// position the plugin window in the main window
					//m_DeviceWinMap[ID]->m_DeviceGUI->GetPluginWindow()->position(px,py);

					if (ps)
					{
						m_DeviceWinMap[ID]->m_DeviceGUI->Maximise();
						// reposition after maximise
						m_DeviceWinMap[ID]->m_DeviceGUI->position(x,y);
					}
					else
						m_DeviceWinMap[ID]->m_DeviceGUI->Minimise();

					if (merge)
						Fl_Canvas::AppendSelection(ID, m_Canvas);
				}
				else if (merge)
					Fl_Canvas::AppendSelection(ID, m_Canvas);

				if (!merge)
					if (m_NextID<=ID)
						m_NextID=ID+1;
			}
			else
			{
				SpiralInfo::Alert("Error in stream, can't really recover data from here on.");
				ThawAll();

				return s;
			}
		}
	}

	const string files = !sidecars.empty() ? sidecars
		: merge ? m_FromFilePath+"_files/" : m_FilePath+"_files/";

	for (size_t n = 0; n < samplers.size(); ++n)
	{
		const int group = OnlySamplerFiles(files, samplers[n].second);
		bool owned = false;

		for (size_t m = 0; m < samplers.size(); ++m)
			owned = owned || samplers[m].second == group;

		if (group >= 0 && !owned)
		{
			SpiralPlugin *sampler = m_DeviceWinMap[samplers[n].first]->m_Device;
			sampler->LoadExternalFiles(files, group);
			sampler->GetChannelHandler()->FlushChannels();
		}
	}

	GrowUnavailablePorts(s, merge, m_DeviceWinMap, m_Copied.m_DeviceIds);

	if (!merge)
	{
		s>>*m_Canvas;
		m_Canvas->CenterPatch();
	}

	ThawAll();

	return s;
}

iostream &operator>>(iostream &s, SynthModular &o)
{
	return o.StreamPatchIn(s, false);
}

//////////////////////////////////////////////////////////

static string BindPath(Fl_Canvas &canvas, int id, bool input, int port)
{
	char text[64];

	snprintf(text, sizeof text, "%s/%d/%s/%d", canvas.IsBox(id) ? "SubPatch" : "Device", id, input ? "Input" : "Output", port);

	return text;
}

void DescribePage(
		Spiral::Core::PatchDescription &patch,
		SynthModular &o,
		const string &sidecars,
		int page,
		bool &external,
		const std::set<int> *only
	)
{
	std::set<string> described;

	for (
			map<int,DeviceWin*>::iterator i = o.m_DeviceWinMap.begin();
			i != o.m_DeviceWinMap.end();
			i++
		)
	{
		if (!i->second->m_DeviceGUI)
			continue;

		if (o.m_Canvas->PageOf(i->first) != page)
			continue;

		if (only && !only->count(i->first))
			continue;

		const bool box = o.m_Canvas->IsBox(i->first);

		if (!i->second->m_Device && i->second->m_PluginID != COMMENT_ID)
			continue;

		char text[32];
		Spiral::Core::ObjectDescription &object = patch.AddObject();

		snprintf(text, sizeof text, "%d", i->second->m_DeviceGUI->x());
		object.SetValue("x", text);
		snprintf(text, sizeof text, "%d", i->second->m_DeviceGUI->y());
		object.SetValue("y", text);
		object.SetValue("collapsed", i->second->m_DeviceGUI->GetPluginWindow()
			&& !i->second->m_DeviceGUI->GetPluginWindow()->visible() ? "true" : "false");

		if (i->second->m_PluginID == COMMENT_ID)
		{
			object.SetValue("text", ((Fl_CommentGUI*)(i->second->m_DeviceGUI))->GetComment());

			continue;
		}

		Spiral::Core::DeviceDescription &device = object.AddDevice();
		snprintf(text, sizeof text, "%d", i->first);
		device.SetValue("patch_id", text);
		described.insert(text);

		if (box)
		{
			object.SetValue("text", i->second->m_DeviceGUI->GetName());
			device.SetValue("name", "SubPatch");
			DescribePage(device.MakeSubPatch(), o, sidecars, i->first, external, NULL);

			if (const SubPatchDevice *described = o.m_Canvas->Box(i->first))
			{
				device.Config = described->Config();

				for (int input = 1; input >= 0; --input)
				{
					const vector<BoundaryPort> &ports = input ? described->Inputs : described->Outputs;

					for (size_t k = 0; k < ports.size(); ++k)
					{
						(input ? device.Inputs : device.Outputs).push_back(Spiral::Core::PortName(ports[k].Name, ports[k].Type));

						if (ports[k].InnerID >= 0)
							device.Binds.push_back(Spiral::Core::PortBind(input != 0, (unsigned)k,
								BindPath(*o.m_Canvas, ports[k].InnerID, input != 0, ports[k].InnerPort)));
					}
				}
			}

			continue;
		}

		object.SetValue("text", SavedDeviceName(i->second));
		snprintf(text, sizeof text, "%d", i->second->m_PluginID);
		device.SetValue("id", text);
		device.SetValue("name", SavedDeviceName(i->second));

		if (Spiral::Panels::PanelWindow *window = dynamic_cast<Spiral::Panels::PanelWindow *>(i->second->m_DeviceGUI->GetPluginWindow()))
			window->SaveState(object.State);

		Spiral::Core::Description described;
		i->second->m_Device->Describe(described);
		Spiral::File::PatchFile::Properties(described, device);

		// save external files
		if (!sidecars.empty() && i->second->m_Device->SaveExternalFiles(sidecars))
		{
			external=true;
		}
	}

	::Describe(patch, *o.m_Canvas, page);

	for (size_t w = patch.Wires.size(); only && w-- > 0; )
		if (!described.count(patch.Wires[w].FromDevice) || !described.count(patch.Wires[w].ToDevice))
			patch.Wires.erase(patch.Wires.begin() + w);
}

void Describe(Spiral::Core::PatchDescription &patch, SynthModular &o, const string &sidecars)
{
	o.FreezeAll();

	patch.Clear();
	patch.Values["application"] = "SpiralSynthModular";
	patch.Voices = o.m_PatchVoices;

	// make external files dir
	bool ExternalDirUsed=false;
	string directoryError;

	Spumoni::Path::MakeDirectories(sidecars, directoryError);

	DescribePage(patch, o, sidecars, -1, ExternalDirUsed, NULL);
	patch.Midi = o.m_MidiMap.Devices;

	// remove it if it wasn't used
	if (!ExternalDirUsed)
	{
		// i guess rmdir won't work if there is something in the dir
		// anyway, but best to be on the safe side. (could do rm -rf) :)
		rmdir(sidecars.c_str());
	}

	o.ThawAll();
}

void SynthModular::Apply(
		const Spiral::Core::PatchDescription &patch,
		const string &sidecars,
		bool merge
	)
{
	FreezeAll();

	std::map<std::string, int> ids;

	if (merge)
		Fl_Canvas::ClearSelection(m_Canvas);

	m_AppliedIds.clear();

	if (!merge)
		SetPatchVoices(patch.Voices);

	if (ApplyPage(patch, sidecars, merge, merge ? m_Canvas->Page() : -1, ids) && !merge)
		m_Canvas->CenterPatch();

	if (!merge)
		m_MidiMap.Devices.clear();

	for (size_t d = 0; d < patch.Midi.size(); ++d)
	{
		std::map<std::string, int>::const_iterator placed = m_AppliedIds.find(patch.Midi[d].Device);

		if (placed == m_AppliedIds.end())
			continue;

		char listed[32];

		snprintf(listed, sizeof listed, "%d", placed->second);
		m_MidiMap.Add(listed);

		for (size_t b = 0; b < patch.Midi[d].Bindings.size(); ++b)
			m_MidiMap.Bind(listed, patch.Midi[d].Bindings[b]);
	}

	m_AppliedIds.clear();

	if (m_MidiPage)
		m_MidiPage->Refresh();

	ThawAll();
	m_Sidecars = sidecars.empty() ? m_FilePath + "_files/" : sidecars;
	RebuildVoices();
}

static bool ParseBindPath(const string &path, string &id, bool &input, int &port)
{
	vector<string> parts;
	string::size_type at = 0;

	for (string::size_type slash; (slash = path.find('/', at)) != string::npos; at = slash + 1)
		parts.push_back(path.substr(at, slash - at));

	parts.push_back(path.substr(at));
	if (parts.size() != 4 || (parts[0] != "Device" && parts[0] != "SubPatch") || (parts[2] != "Input" && parts[2] != "Output"))
		return false;

	id = parts[1];
	input = parts[2] == "Input";
	port = atoi(parts[3].c_str());

	return true;
}

bool SynthModular::ApplyPage(
		const Spiral::Core::PatchDescription &patch,
		const string &sidecars,
		bool merge,
		int page,
		std::map<std::string, int> &ids
	)
{
	const bool shown = page == m_Canvas->Page();

	for (size_t n = 0; !merge && n < patch.Objects.size(); n++)
		for (size_t d = 0; d < patch.Objects[n].Devices.size(); d++)
		{
			const string PatchID = patch.Objects[n].Devices[d].Value("patch_id");
			char *end = NULL;
			const int ID = (int)strtol(PatchID.c_str(), &end, 10);
			if (!PatchID.empty() && !*end && ID >= 0 && m_DeviceWinMap.find(ID) == m_DeviceWinMap.end() && m_NextID <= ID)
				m_NextID = ID + 1;
		}

	for (size_t n = 0; n < patch.Objects.size(); n++)
	{
		const Spiral::Core::ObjectDescription &object = patch.Objects[n];
		const int x = atoi(object.Value("x").c_str()), y = atoi(object.Value("y").c_str());

		if (object.IsComment())
		{
			DeviceWin *temp = NewComment(COMMENT_ID, x, y);

			if (!temp)
				continue;

			const int ID = m_NextID++;
			temp->m_DeviceGUI->SetID(ID);
			m_DeviceWinMap[ID] = temp;
			m_Canvas->SetPageOf(ID, page);
			((Fl_CommentGUI*)(temp->m_DeviceGUI))->SetComment(object.Value("text"));

			if (!shown)
				temp->m_DeviceGUI->hide();
			else if (merge)
				Fl_Canvas::AppendSelection(ID, m_Canvas);

			continue;
		}

		for (size_t d = 0; d < object.Devices.size(); d++)
		{
			const Spiral::Core::DeviceDescription &device = object.Devices[d];
			const string PatchID = device.Value("patch_id");
			const int PluginID = atoi(device.Value("id").c_str());
			const string Name = device.Value("name");

			if (ids.find(PatchID) != ids.end())
			{
				SpiralInfo::Alert("Duplicate device ID found in file - aborting load");

				return false;
			}

			char *end = NULL;
			int ID = (int)strtol(PatchID.c_str(), &end, 10);
			if (merge || PatchID.empty() || *end || ID < 0 || m_DeviceWinMap.find(ID) != m_DeviceWinMap.end())
				ID = m_NextID++;

			ids[PatchID] = ID;
			m_AppliedIds[PatchID] = ID;

			if (!merge && m_NextID <= ID)
				m_NextID = ID + 1;

			if (device.IsSubPatch())
			{
				SubPatchDevice *box = new SubPatchDevice;

				for (size_t k = 0; k < device.Inputs.size(); ++k)
					box->Inputs.push_back(BoundaryPort(-1, -1, device.Inputs[k].Name, device.Inputs[k].Type));

				for (size_t k = 0; k < device.Outputs.size(); ++k)
					box->Outputs.push_back(BoundaryPort(-1, -1, device.Outputs[k].Name,
						device.Outputs[k].Type == "MusicalEventPort" ? string() : device.Outputs[k].Type));

				for (size_t b = 0; b < device.Binds.size(); ++b)
				{
					vector<BoundaryPort> &ports = device.Binds[b].OuterIsInput ? box->Inputs : box->Outputs;
					if (ports.size() <= device.Binds[b].OuterIndex)
						ports.resize(device.Binds[b].OuterIndex + 1);
				}

				const string title = object.Value("text").empty() ? string("SubPatch") : object.Value("text");
				DeviceWin *temp = NewSubPatch(x, y, title, box, device.Config);
				temp->m_DeviceGUI->SetID(ID);
				if (temp->m_DeviceGUI->GetPluginWindow() && object.Value("collapsed") == "false")
				{
					temp->m_DeviceGUI->Maximise();
					temp->m_DeviceGUI->position(x, y);
				}

				m_DeviceWinMap[ID] = temp;
				m_Canvas->SetPageOf(ID, page);
				m_Canvas->SetBox(ID, box);
				std::map<std::string, int> inner;

				if (device.Inner() && !ApplyPage(*device.Inner(), sidecars, merge, ID, inner))
					return false;

				for (size_t b = 0; b < device.Binds.size(); ++b)
				{
					string innerId;
					bool input;
					int port;
					std::map<std::string, int>::const_iterator found;

					if (!ParseBindPath(device.Binds[b].InnerPath, innerId, input, port)
					    || (found = inner.find(innerId)) == inner.end() || input != device.Binds[b].OuterIsInput)
					{
						cerr << "SSM: SubPatch bind " << device.Binds[b].InnerPath << " leads nowhere.\n";

						continue;
					}

					if (!input && m_Canvas->IsEventPort(found->second, false, port))
					{
						cerr << "SSM: A SubPatch sends no events out; " << device.Binds[b].InnerPath << " is not bound.\n";

						continue;
					}

					BoundaryPort &bound = (input ? box->Inputs : box->Outputs)[device.Binds[b].OuterIndex];
					bound.InnerID = found->second;
					bound.InnerPort = port;
				}

				m_Canvas->SetBox(ID, box);

				if (!shown)
					temp->m_DeviceGUI->hide();
				else if (merge)
					Fl_Canvas::AppendSelection(ID, m_Canvas);

				continue;
			}

			DeviceWin *temp = NewDeviceWin(PluginID, x, y);

			if (!temp)
			{
				SpiralInfo::Alert("Error in patch, can't really recover data from here on.");

				return false;
			}

			temp->m_DeviceGUI->SetID(ID);
			temp->m_DeviceGUI->SetName(Name);

			if (UnavailablePlugin *missingDevice = dynamic_cast<UnavailablePlugin*>(temp->m_Device))
			{
				missingDevice->SetSavedName(Name);
				string label = Name.empty() ? string("Plugin") : Name;
				temp->m_DeviceGUI->SetName(label + " (unavailable)");
				cerr << "SSM: Missing plugin " << label << " (" << PluginID
					<< "); preserving its state and wires.\n";
			}

			temp->m_Device->SetUpdateInfoCallback(ID, cb_UpdatePluginInfo);
			m_DeviceWinMap[ID] = temp;
			m_Canvas->SetPageOf(ID, page);

			Spiral::Core::Description state;
			Spiral::File::PatchFile::Describe(device, state);
			temp->m_Device->Migrate(state);
			Spiral::Core::Description::Reader values(state);
			temp->m_Device->Apply(values);

			const int savedID = merge && !PatchID.empty() && !*end ? (int)strtol(PatchID.c_str(), NULL, 10) : -1;
			temp->m_Device->LoadExternalFiles(sidecars.empty() ? m_FilePath + "_files/" : sidecars, savedID);

			temp->m_Device->GetChannelHandler()->FlushChannels();

			if (temp->m_DeviceGUI->GetPluginWindow())
			{
				if (SpiralPluginGUI *gui = dynamic_cast<SpiralPluginGUI *>(temp->m_DeviceGUI->GetPluginWindow()))
					gui->UpdateValues(temp->m_Device);

				Spiral::Panels::PanelWindow *window = dynamic_cast<Spiral::Panels::PanelWindow *>(temp->m_DeviceGUI->GetPluginWindow());
				if (window)
					window->LoadState(object.State);

				if (object.Value("collapsed") == "true")
				{
					temp->m_DeviceGUI->Minimise();
				}
				else
				{
					temp->m_DeviceGUI->Maximise();
					temp->m_DeviceGUI->position(x, y);
				}
			}

			if (!shown)
				temp->m_DeviceGUI->hide();
			else if (merge)
				Fl_Canvas::AppendSelection(ID, m_Canvas);
		}
	}

	for (size_t w = 0; w < patch.Wires.size(); ++w)
	{
		std::map<std::string, int>::const_iterator from = ids.find(patch.Wires[w].FromDevice);
		std::map<std::string, int>::const_iterator to = ids.find(patch.Wires[w].ToDevice);

		if (from != ids.end())
			NoteUnavailablePort(m_DeviceWinMap, from->second, false, atoi(patch.Wires[w].FromPort.c_str()));

		if (to != ids.end())
			NoteUnavailablePort(m_DeviceWinMap, to->second, true, atoi(patch.Wires[w].ToPort.c_str()));
	}

	::Apply(patch, *m_Canvas, ids);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

// Callbacks

/////////////////////////////////
// File Menu & associated buttons

// New

inline void SynthModular::cb_New_i (Fl_Widget *o, void *v)
{
	if (m_DeviceWinMap.size()>0 && !Pawfal_YesNo ("New - Lose changes to current patch?"))
		return;

	m_TopWindow->label (TITLEBAR.c_str());
	ClearUp();
	m_SavePointCount = 0;
	m_FilePath.clear();
	m_BranchID.clear();
	m_Document = Spiral::File::DocumentSection();

	if (m_Drawer)
		m_Drawer->Refresh();
}

void SynthModular::cb_New (Fl_Widget *o, void *v)
{
	((SynthModular*)(o->user_data()))->cb_New_i (o, v);
}

// Load

void SynthModular::ChooseAndLoadPatch(const char *directory)
{
	if (m_DeviceWinMap.size()>0 && !Pawfal_YesNo ("Load - Lose changes to current patch?"))
		return;

	char *fn=fl_file_chooser (directory ? "Load an example patch" : "Load a patch",
		"SSM Patches (*.{ssm,ssmp})\tSSM Project (*.ssmp)\tLegacy SSM (*.ssm)", directory);

	if (fn && *fn!='\0')
		LoadPatch(fn);
}

inline void SynthModular::cb_Load_i (Fl_Widget *o, void *v)
{
	ChooseAndLoadPatch(NULL);
}

static std::string ExamplesDirectory()
{
	struct stat info;

	if (SSM_EXAMPLES_DIR[0] && stat(SSM_EXAMPLES_DIR, &info) == 0 && S_ISDIR(info.st_mode))
		return SSM_EXAMPLES_DIR;

	return SpiralBundleResourceDirectory("Examples");
}

void SynthModular::cb_Examples(Fl_Widget *o, void *v)
{
	std::string directory = ExamplesDirectory();

	if (directory.empty())
	{
		fl_message("Examples were not found under the install prefix or the application bundle.");

		return;
	}

	if (directory[directory.size() - 1] != '/')
		directory += "/";

	((SynthModular*)v)->ChooseAndLoadPatch(directory.c_str());
}

void SynthModular::cb_Load(Fl_Widget *o, void *v)
{
	((SynthModular*)(o->user_data()))->cb_Load_i (o, v);
}

// Save

inline void SynthModular::cb_Save_i (Fl_Widget *o, void *v)
{
	if (m_FilePath.empty())
	{
		cb_SaveAs_i(o, v);

		return;
	}

	if (Spiral::File::Project::PathLooksLikePackage(m_FilePath))
		SaveBranch();
	else
		cb_SaveAs_i(o, v);
}

void SynthModular::cb_Save (Fl_Widget *o, void *v)
{
	((SynthModular*)(o->user_data()))->cb_Save_i (o, v);
}

inline void SynthModular::cb_SaveAs_i (Fl_Widget *o, void *v)
{
	char *fn=fl_file_chooser("Save a patch", "SSM Project (*.ssmp)", NULL);
	if (fn && *fn!='\0')
	{
		ifstream ifl (fn);

		if (ifl)
		{
			if (!Pawfal_YesNo ("File [%s] exists, overwrite?", fn))
				return;
		}

		// Same package: overwrite the open branch and keep the others.
		if (Spiral::File::Project::PathLooksLikePackage(fn)
		    && Spumoni::Project::SameFile(m_FilePath, fn))
		{
			SaveBranch(false);

			return;
		}

		SavePatch(fn);
	}
}

void SynthModular::cb_SaveAs (Fl_Widget *o, void *v)
{
	((SynthModular*)(o->user_data()))->cb_SaveAs_i (o, v);
}

// Merge

inline void SynthModular::cb_Merge_i (Fl_Widget *o, void *v)
{
	char *fn = fl_file_chooser ("Merge a patch",
	                            "SSM Patches (*.{ssm,ssmp})\tSSM Project (*.ssmp)\tLegacy SSM (*.ssm)", NULL);

	if (fn && *fn!='\0')
		MergePatch(fn);
}

void SynthModular::cb_MergeAsSubPatch (Fl_Widget *o, void *v)
{
	SynthModular *synth = (SynthModular *)v;
	const char *fn = fl_file_chooser ("Merge a patch as a subpatch",
	                                  "SSM Patches (*.{ssm,ssmp})\tSSM Project (*.ssmp)\tLegacy SSM (*.ssm)", NULL);

	if (!fn || !*fn)
		return;

	const string path = fn;
	const size_t slash = path.find_last_of("/\\");
	string name = slash == string::npos ? path : path.substr(slash + 1);
	const size_t dot = name.find_last_of('.');
	if (dot != string::npos && dot > 0)
		name = name.substr(0, dot);

	const char *title = fl_input("Group name:", name.c_str());
	if (!title)
		return;

	const string group = title;
	const int before = synth->m_NextID;
	synth->MergePatch(path.c_str());
	if (synth->m_NextID != before)
		synth->GroupSelection(group);
}

void SynthModular::OpenDropped(const string &path)
{
	const size_t dot = path.find_last_of('.');
	string extension = dot == string::npos ? string() : path.substr(dot);

	for (size_t i = 0; i < extension.size(); ++i)
		extension[i] = tolower((unsigned char)extension[i]);

	if (!Spiral::File::Project::PathLooksLikePackage(path) && extension != ".ssm")
	{
		fl_message("%s is not an SSM patch.\nPatches are .ssmp projects or legacy .ssm files.", path.c_str());

		return;
	}

	if (m_DeviceWinMap.empty())
	{
		LoadPatch(path.c_str());

		return;
	}

	const size_t slash = path.find_last_of("/\\");
	const string name = slash == string::npos ? path : path.substr(slash + 1);

	switch (fl_choice("Merge %s into this patch, or open it in place of this patch?\n"
	                  "Opening it loses changes to this patch that are not saved.",
	                  "Cancel", "Merge", "Open", name.c_str()))
	{
		case 1:
			MergePatch(path.c_str());

			break;
		case 2:
			LoadPatch(path.c_str());

			break;
	}
}

void SynthModular::cb_DropFile(Fl_Canvas *o, void *v)
{
	((SynthModular *)v)->OpenDropped(o->DroppedFile());
}

void SynthModular::MergePatch(const char *fn)
{
	iostream *stream = NULL;
	ifstream in;
	fstream inf;
	std::stringstream packaged;
	std::auto_ptr<Spiral::File::Project> project;
	std::string sidecars;

	if (Spiral::File::Project::PathLooksLikePackage(fn))
	{
		project.reset(new Spiral::File::Project(fn));
		std::string error;

		if (!project->OpenPackage("", error))
		{
			fl_message("%s", error.c_str());

			return;
		}

		if (project->Source().Empty() && !project->HasPatch())
		{
			fl_message("This SSM Project has no patch in it.");

			return;
		}

		packaged.str(project->Source().Bytes());
		stream = &packaged;
		sidecars = project->SidecarDirectory();
	}
	else
	{
		in.open(fn);
		if (!in)
			return;

		inf.open(fn, ios::in);
		stream = &inf;
	}

	m_MergeFilePath = fn;
	std::string json = project.get() && project->HasPatch() ? project->Patch().Bytes() : JSONDocument(*stream);
	if (!json.empty())
	{
		Spiral::Core::PatchDescription patch;
		std::string error;

		if (!Spiral::File::PatchFile::ReadText(json, patch, error))
		{
			fl_message("%s", error.c_str());

			return;
		}

		Apply(patch, sidecars, true);
	}
	else
	{
		StreamPatchIn(*stream, true, sidecars);
		m_Canvas->StreamSelectionWiresIn(*stream, m_Copied.m_DeviceIds, true);
	}

	if (stream == &inf)
		inf.close();
}

void SynthModular::cb_Merge (Fl_Widget *o, void *v)
{
	((SynthModular*)(o->parent()->user_data()))->cb_Merge_i (o, v);
}

// Close

inline void SynthModular::cb_Close_i (Fl_Widget *o, void *v)
{
	m_SettingsWindow->hide();
	m_TopWindow->hide();
}

void SynthModular::cb_Close (Fl_Widget *o, void *v)
{
	((SynthModular*)(o->user_data()))->cb_Close_i (o, v);
}

/////////////////////////////////
// Edit Menu

// Cut

inline void SynthModular::cb_Cut_i(Fl_Widget *o, void *v)
{
	if (! m_Canvas->HaveSelection())
		return;

	// show some warning here
	cb_Copy_i (o, v);  // should we be calling an inline function here??????

	for (unsigned int i=0; i<m_Canvas->Selection().m_DeviceIds.size(); i++)
	{
		int ID = m_Canvas->Selection().m_DeviceIds[i];
		std::map<int,DeviceWin*>::iterator found = m_DeviceWinMap.find(ID);

		if (found == m_DeviceWinMap.end() || !found->second || !found->second->m_DeviceGUI)
			continue;

		Fl_DeviceGUI::Kill(found->second->m_DeviceGUI);
	}

	Fl_Canvas::ClearSelection(m_Canvas);
}

void SynthModular::cb_Cut (Fl_Widget *o, void *v)
{
	if (!v)
		return;

	((SynthModular*)v)->cb_Cut_i (o, v);
}

// Copy

inline void SynthModular::cb_Copy_i (Fl_Widget *o, void *v)
{
	if (! m_Canvas->HaveSelection())
		return;

	const CanvasGroup selection = m_Canvas->Selection();
	const std::set<int> only(selection.m_DeviceIds.begin(), selection.m_DeviceIds.end());
	bool external = false;
	FreezeAll();
	m_CopiedPatch.Clear();
	DescribePage(m_CopiedPatch, *this, string(), m_Canvas->Page(), external, &only);
	ThawAll();
	m_CopiedFrom = m_FilePath;
	Fl_Canvas::EnablePaste (m_Canvas);
}

void SynthModular::cb_Copy (Fl_Widget *o, void *v)
{
	if (!v)
		return;

	((SynthModular*)v)->cb_Copy_i (o, v);
}

// Paste

inline void SynthModular::cb_Paste_i (Fl_Widget *o, void *v)
{
	PasteCopied(false);
}

void SynthModular::cb_Paste (Fl_Widget *o, void *v)
{
	if (!v)
		return;

	((SynthModular*)v)->cb_Paste_i (o, v);
}

void SynthModular::cb_PasteAsSubPatch (Fl_Widget *o, void *v)
{
	if (!v)
		return;

	((SynthModular*)v)->PasteCopied(true);
}

static const int PASTE_OFFSET = 20;

static unsigned OmitHostIO(Spiral::Core::PatchDescription &patch)
{
	std::set<string> gone;

	for (size_t n = patch.Objects.size(); n-- > 0; )
	{
		vector<Spiral::Core::DeviceDescription> &devices = patch.Objects[n].Devices;

		for (size_t d = devices.size(); d-- > 0; )
			if (HostIO(atoi(devices[d].Value("id").c_str())))
			{
				gone.insert(devices[d].Value("patch_id"));
				devices.erase(devices.begin() + d);
				if (devices.empty())
					patch.Objects.erase(patch.Objects.begin() + n);
			}
	}

	for (size_t w = patch.Wires.size(); w-- > 0; )
		if (gone.count(patch.Wires[w].FromDevice) || gone.count(patch.Wires[w].ToDevice))
			patch.Wires.erase(patch.Wires.begin() + w);

	return (unsigned)gone.size();
}

void SynthModular::PasteCopied(bool grouped)
{
	if (m_CopiedPatch.Objects.empty())
		return;

	string title;

	if (grouped)
	{
		const char *name = fl_input("Group name:", "Group");
		if (!name)
			return;

		title = name;
	}

	Spiral::Core::PatchDescription pasted(m_CopiedPatch);

	for (size_t n = 0; n < pasted.Objects.size(); ++n)
	{
		Spiral::Core::ObjectDescription &object = pasted.Objects[n];
		char at[32];

		snprintf(at, sizeof at, "%d", atoi(object.Value("x").c_str()) + PASTE_OFFSET);
		object.SetValue("x", at);
		snprintf(at, sizeof at, "%d", atoi(object.Value("y").c_str()) + PASTE_OFFSET);
		object.SetValue("y", at);
	}

	if (m_Canvas->Page() != -1 && OmitHostIO(pasted))
	{
		if (pasted.Objects.empty())
		{
			fl_message("Jack, Output, MIDI and Keyboard can't be pasted into a SubPatch.");

			return;
		}

		fl_message("Jack, Output, MIDI and Keyboard were not pasted into the SubPatch.");
	}

	Apply(pasted, m_CopiedFrom.empty() ? string() : m_CopiedFrom + "_files/", true);

	if (grouped)
		GroupSelection(title);
}

// Delete

inline void SynthModular::cb_Delete_i (Fl_Widget *o, void *v)
{
	m_Canvas->DeleteSelection();
}

void SynthModular::cb_Delete (Fl_Widget *o, void *v)
{
	((SynthModular*)(o->user_data()))->cb_Delete_i (o, v);
}

// Options

inline void SynthModular::cb_Options_i (Fl_Widget *o, void *v)
{
	m_SettingsWindow->show();
}

void SynthModular::cb_CenterPatch (Fl_Widget* o, void* v)
{
	((SynthModular*)(o->user_data()))->cb_CenterPatch_i (o, v);
}

inline void SynthModular::cb_CenterPatch_i (Fl_Widget *o, void *v)
{
	m_Canvas->CenterPatch();
}

void SynthModular::cb_Options (Fl_Widget* o, void* v)
{
	((SynthModular*)(o->user_data()))->cb_Options_i (o, v);
}

void SynthModular::cb_Relayout (void* v)
{
	((SynthModular*)v)->LayoutChrome();
}

void SynthModular::cb_AddEvents (void* v)
{
	((SynthModular*)v)->AddEventInput();
}

void SynthModular::cb_DrawerCollapse (void* v)
{
	((SynthModular*)v)->LayoutChrome();
}

inline void SynthModular::cb_AboutPatch_i (Fl_Widget *o, void *v)
{
	if (!m_Drawer)
		return;

	m_Drawer->ShowMidi(false);
	if (m_Drawer->Collapsed())
		m_Drawer->SetCollapsed(false);
	else
		m_Drawer->Refresh();

	LayoutChrome();
}

void SynthModular::cb_AboutPatch (Fl_Widget* o, void* v)
{
	((SynthModular*)v)->cb_AboutPatch_i (o, v);
}

inline void SynthModular::cb_MidiBindings_i (Fl_Widget *o, void *v)
{
	if (!m_Drawer)
		return;

	m_Drawer->ShowMidi(true);
	if (m_Drawer->Collapsed())
		m_Drawer->SetCollapsed(false);
	else
		m_Drawer->Refresh();

	if (m_MidiPage)
		m_MidiPage->Refresh();

	LayoutChrome();
}

void SynthModular::cb_MidiBindings (Fl_Widget* o, void* v)
{
	((SynthModular*)v)->cb_MidiBindings_i (o, v);
}

std::string SynthModular::cb_MidiDeviceName (const std::string &device, void *v)
{
	SynthModular *synth = (SynthModular*)v;
	map<int,DeviceWin*>::iterator i = synth->m_DeviceWinMap.find(atoi(device.c_str()));

	if (i == synth->m_DeviceWinMap.end() || !i->second->m_DeviceGUI)
		return string();

	return i->second->m_DeviceGUI->GetName() + " " + device;
}

void SynthModular::cb_PickMidiDevice (void *v)
{
	SynthModular *synth = (SynthModular*)v;
	if (synth->m_Canvas->Picking() || synth->m_PickDevice >= 0)
		synth->StopMidiPick();
	else
		synth->PickMidiDevice();
}

void SynthModular::cb_PickMidiControl (const std::string &device, void *v)
{
	((SynthModular*)v)->PickMidiControl(atoi(device.c_str()));
}

void SynthModular::cb_LearnMidi (void (*learned)(int, int, void *), void *learnData, void *v)
{
	((SynthModular*)v)->LearnMidi(learned, learnData);
}

void SynthModular::cb_MidiSource (const std::string &source, void *v)
{
	SpiralInfo::MIDIFILE = source;
	((SynthModular*)v)->m_Info.MIDIFILE = source;
	Spiral::Core::MidiDevice::SetDeviceName(source);
}

void SynthModular::PickMidiDevice()
{
	StopMidiPick();
	m_Canvas->StartPick(cb_DeviceAt, cb_DevicePicked, this, "Click a device to bind its controls to MIDI \xc2\xb7 Esc to stop");

	if (m_MidiPage)
		m_MidiPage->SetPicking(true);
}

void SynthModular::PickMidiControl(int id)
{
	map<int,DeviceWin*>::iterator i = m_DeviceWinMap.find(id);

	if (i == m_DeviceWinMap.end() || !i->second->m_DeviceGUI)
		return;

	if (m_PickDevice != id)
	{
		StopMidiPick();
		Fl_DeviceGUI *gui = i->second->m_DeviceGUI;
		m_PickOpened = gui->IsMinimised();
		if (m_PickOpened)
			gui->Maximise();

		m_PickDevice = id;
		m_Canvas->Isolate(id);
		m_Canvas->CenterOn(id);
	}

	m_Canvas->StartPick(cb_ControlAt, cb_ControlPicked, this,
		"Click a control to bind it to MIDI \xc2\xb7 Esc when done");

	if (m_MidiPage)
		m_MidiPage->SetPicking(true);
}

void SynthModular::StopMidiPick()
{
	if (m_Canvas && m_Canvas->Picking())
		m_Canvas->StopPick();

	if (m_Canvas)
		m_Canvas->Isolate(-1);

	if (m_PickDevice >= 0 && m_PickOpened)
	{
		map<int,DeviceWin*>::iterator i = m_DeviceWinMap.find(m_PickDevice);

		if (i != m_DeviceWinMap.end() && i->second->m_DeviceGUI)
			i->second->m_DeviceGUI->Minimise();
	}

	m_PickDevice = -1;
	m_PickOpened = false;

	if (m_MidiPage)
		m_MidiPage->SetPicking(false);
}

static Spiral::Panels::Panel *PanelOf(DeviceWin *win)
{
	if (!win || !win->m_DeviceGUI || !win->m_Device)
		return NULL;

	Spiral::Panels::PanelWindow *window = dynamic_cast<Spiral::Panels::PanelWindow *>(win->m_DeviceGUI->GetPluginWindow());

	return window ? window->GetPanel() : NULL;
}

bool SynthModular::cb_DeviceAt (Fl_Canvas *canvas, int x, int y, Fl_Canvas::Pick &out, void *v)
{
	SynthModular *synth = (SynthModular*)v;

	for (int n = canvas->children(); n-- > 0; )
	{
		Fl_DeviceGUI *gui = dynamic_cast<Fl_DeviceGUI*>(canvas->child(n));
		if (!gui || !gui->visible() || x < gui->x() || y < gui->y() || x >= gui->x() + gui->w() || y >= gui->y() + gui->h())
			continue;

		map<int,DeviceWin*>::iterator i = synth->m_DeviceWinMap.find(gui->GetID());

		if (i == synth->m_DeviceWinMap.end() || !PanelOf(i->second))
			return false;

		out.Device = gui->GetID();
		out.X = gui->x();
		out.Y = gui->y();
		out.W = gui->w();
		out.H = gui->h();

		return true;
	}

	return false;
}

void SynthModular::cb_DevicePicked (Fl_Canvas *canvas, const Fl_Canvas::Pick &pick, void *v)
{
	SynthModular *synth = (SynthModular*)v;

	if (pick.Device < 0)
	{
		synth->StopMidiPick();

		return;
	}

	char listed[32];

	snprintf(listed, sizeof listed, "%d", pick.Device);
	synth->m_MidiMap.Add(listed);
	if (synth->m_MidiPage)
		synth->m_MidiPage->Refresh();

	synth->PickMidiControl(pick.Device);
}

bool SynthModular::cb_ControlAt (Fl_Canvas *canvas, int x, int y, Fl_Canvas::Pick &out, void *v)
{
	SynthModular *synth = (SynthModular*)v;
	map<int,DeviceWin*>::iterator i = synth->m_DeviceWinMap.find(synth->m_PickDevice);
	Spiral::Panels::Panel *panel = i == synth->m_DeviceWinMap.end() ? NULL : PanelOf(i->second);
	if (!panel)
		return false;

	int area = -1;

	for (size_t n = 0; n < panel->ControlCount(); ++n)
	{
		Spiral::Panels::PanelControl *control = panel->Control(n);
		Fl_Widget *w = Spiral::Panels::FltkView::WidgetOf(*control);
		if (!w || !w->visible_r() || control->Name().empty() || !control->Drivable())
			continue;

		if (x < w->x() || y < w->y() || x >= w->x() + w->w() || y >= w->y() + w->h())
			continue;

		if (area >= 0 && w->w() * w->h() >= area)
			continue;

		area = w->w() * w->h();
		out.Device = synth->m_PickDevice;
		out.Control = control->Name();
		out.X = w->x();
		out.Y = w->y();
		out.W = w->w();
		out.H = w->h();
	}

	return area >= 0;
}

void SynthModular::cb_ControlPicked (Fl_Canvas *canvas, const Fl_Canvas::Pick &pick, void *v)
{
	SynthModular *synth = (SynthModular*)v;

	if (pick.Device < 0)
	{
		synth->StopMidiPick();

		return;
	}

	char listed[32];

	snprintf(listed, sizeof listed, "%d", pick.Device);

	if (synth->m_MidiPage)
		synth->m_MidiPage->EditBinding(listed, pick.Control);

	synth->PickMidiControl(pick.Device);
}

inline void SynthModular::cb_About_i (Fl_Widget *o, void *v)
{
	const int Pad = 16;
	const int IconSize = 64;
	const int TextX = Pad + IconSize + Pad;
	const int Width = 440;
	const int LineHeight = 16;
	const int ButtonHeight = 26;
	const string version = "Version " + VER_STRING;
	const char *lines[] = {
		"SpiralSynthModular",
		version.c_str(),
		"",
		"We are the music makers",
		"We are the dreamers of dreams",
		"",
		"Copyright \xc2\xa9 Dave Griffiths 2002-2006 and contributors",
		"http://www.pawfal.org",
		"GNU General Public License, version 2 or later",
	};
	const int Lines = sizeof(lines) / sizeof(lines[0]);
	const int TextHeight = Lines * LineHeight;
	const int Height = Pad + TextHeight + Pad + ButtonHeight + Pad;

	Fl_Double_Window About (Width, Height, "About SpiralSynthModular");
	About.position ((Fl::w()/2) - (Width/2), (Fl::h()/2) - (Height/2));
	About.color (SpiralInfo::GUICOL_Tool);

	Fl_Pixmap icon (SpiralIcon_xpm);
	Fl_Box *iconbox = new Fl_Box (Pad, Pad, IconSize, IconSize, "");
	iconbox->box (FL_NO_BOX);
	icon.label (iconbox);

	int y = Pad;

	for (int i = 0; i < Lines; ++i, y += LineHeight)
	{
		Fl_Box *line = new Fl_Box (TextX, y, Width - TextX - Pad, LineHeight, lines[i]);
		line->box (FL_NO_BOX);
		line->labelsize (i == 0 ? 14 : 11);

		if (i == 0)
			line->labelfont (FL_BOLD);

		line->align (FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
	}

	Fl_Return_Button ok (Width - Pad - 80, Height - Pad - ButtonHeight, 80, ButtonHeight, "OK");
	ok.box (FL_PLASTIC_UP_BOX);
	ok.labelsize (11);
	ok.callback (cb_CloseAbout, &About);

	About.end();
	About.set_modal();
	About.show();

	while (About.shown())
		Fl::wait();
}

void SynthModular::cb_About (Fl_Widget* o, void* v)
{
	((SynthModular*)v)->cb_About_i (o, v);
}

void SynthModular::cb_CloseAbout (Fl_Widget* o, void* v)
{
	((Fl_Window*)v)->hide();
}

/////////////////////////////////
// Plugin Menu

// This callback has the name that the callback for the canvas menu
// used to have please note - that is now NewDeviceFromCanvasMenu

inline void SynthModular::cb_NewDeviceFromMenu_i (Fl_Widget *o, void *v)
{
	AddDevice (*((int*)v));
}

void SynthModular::cb_NewDeviceFromMenu (Fl_Widget *o, void *v)
{
	((SynthModular*)(o->user_data()))->cb_NewDeviceFromMenu_i (o, v);
}

// (Plugin Buttons)

inline void SynthModular::cb_NewDevice_i (Fl_Button *o, void *v)
{
	AddDevice (*((int*)v));
}

void SynthModular::cb_NewDevice (Fl_Button *o, void *v)
{
	((SynthModular*)(o->parent()->user_data()))->cb_NewDevice_i (o, v);
}

// (Plugin Canvas Menu)

inline void SynthModular::cb_NewDeviceFromCanvasMenu_i (Fl_Canvas* o, void* v)
{
	AddDevice(*((int*)v),*((int*)v+1),*((int*)v+2));
}

void SynthModular::cb_NewDeviceFromCanvasMenu(Fl_Canvas* o, void* v)
{
	((SynthModular*)(o->user_data()))->cb_NewDeviceFromCanvasMenu_i(o,v);
}

/////////////////////////////////
// Audio Menu

// Play / Pause

inline void SynthModular::cb_PlayPause_i (Fl_Widget *o, void *v)
{
	string oldname = m_PlayPause->tooltip ();

	if (m_Info.PAUSED)
	{
		m_PlayPause->label ("@||");
		m_PlayPause->tooltip ("Pause");
		ResumeAudio();
	}
	else
	{
		m_PlayPause->label ("@>");
		m_PlayPause->tooltip ("Play");
		PauseAudio();
	}

	MenuRelabel (oldname, m_PlayPause->tooltip());
}

void SynthModular::cb_PlayPause (Fl_Widget *o, void *v)
{
	((SynthModular*)(o->user_data()))->cb_PlayPause_i (o, v);
}

// Reset

inline void SynthModular::cb_Reset_i (Fl_Widget *o, void *v)
{
	ResetAudio();
}

void SynthModular::cb_Reset (Fl_Widget *o, void *v)
{
	((SynthModular*)(o->user_data()))->cb_Reset_i (o, v);
}

//////////////////////////////////////////////////////////

inline void SynthModular::cb_NewComment_i(Fl_Button* o, void* v)
{
	AddComment(-1);
}

void SynthModular::cb_NewComment(Fl_Button* o, void* v)
{
	((SynthModular*)(o->parent()->user_data()))->cb_NewComment_i(o,v);
}

//////////////////////////////////////////////////////////

inline void SynthModular::cb_GroupTab_i(Fl_Tabs* o, void* v)
{
	m_GroupTab->redraw();
}

void SynthModular::cb_GroupTab(Fl_Tabs* o, void* v)
{
	((SynthModular*)(o->parent()->user_data()))->cb_GroupTab_i(o,v);
}

//////////////////////////////////////////////////////////

inline void SynthModular::cb_Connection_i(Fl_Canvas* o, void* v)
{
	CanvasWire *Wire=(CanvasWire*)v;
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

	if (si->second->m_Device->IsEventPort(false, Wire->OutputPort) != di->second->m_Device->IsEventPort(true, Wire->InputPort))
		return;

	if (si->second->m_Device->IsEventPort(false, Wire->OutputPort))
	{
		di->second->m_Device->SetEventInput(Wire->InputPort, si->second->m_Device->GetEventOutput(Wire->OutputPort));

		return;
	}

	if (!si->second->m_Device->GetOutput(Wire->OutputPort,&sample))
	{
		char num[32];
		sprintf(num,"%d,%d",Wire->OutputID,Wire->OutputPort);
		SpiralInfo::Alert("Warning: Connection problem - can't find source output "+string(num));

		return;
	}

	LADSPAPlugin *effect = dynamic_cast<LADSPAPlugin*>(si->second->m_Device);
	if (effect && !effect->HasEffect())
		sample = NULL;

	if (!di->second->m_Device->SetInput(Wire->InputPort,(const Sample*)sample))
	{
		char num[32];
		sprintf(num,"%d,%d",Wire->InputID,Wire->InputPort);
		SpiralInfo::Alert("Warning: Connection problem - can't find source input "+string(num));

		return;
	}
}

void SynthModular::cb_Connection(Fl_Canvas* o, void* v)
{
	((SynthModular*)(o->user_data()))->cb_Connection_i(o,v);
}

//////////////////////////////////////////////////////////

inline void SynthModular::cb_Unconnect_i(Fl_Canvas* o, void* v)
{
	CanvasWire *Wire=(CanvasWire*)v;

	//cerr<<Wire->InputID<<" "<<Wire->InputPort<<endl;

	map<int,DeviceWin*>::iterator di=m_DeviceWinMap.find(Wire->InputID);

	if (di==m_DeviceWinMap.end())
	{
		//cerr<<"Can't find destination device "<<Wire->InputID<<endl;
		return;
	}

	SpiralPlugin *Plugin=di->second->m_Device;
	if (Plugin)
		Plugin->SetEventInput(Wire->InputPort,NULL);

	if (Plugin && !Plugin->SetInput(Wire->InputPort,NULL))
	{
		cerr<<"Can't find destination device's Input"<<endl;

		return;
	}
}

void SynthModular::cb_Unconnect(Fl_Canvas* o, void* v)
{
	((SynthModular*)(o->user_data()))->cb_Unconnect_i(o,v);
}

//////////////////////////////////////////////////////////

void SynthModular::cb_UpdatePluginInfo(int ID, void *PInfo)
{
	map<int,DeviceWin*>::iterator i=m_DeviceWinMap.find(ID);

	if (i!=m_DeviceWinMap.end())
	{
		DeviceGUIInfo Info=BuildDeviceGUIInfo(*((PluginInfo*)PInfo));

		StablePortLayout *stable = dynamic_cast<StablePortLayout*>((*i).second->m_Device);
		vector<int> moved;

		for (int p=0; stable && p<(*i).second->m_DeviceGUI->GetInfo()->NumInputs; p++)
			moved.push_back(stable->MovedInput(p));

		(*i).second->m_DeviceGUI->SetupPorts(Info, false, stable != NULL, stable ? &moved : NULL);
		(*i).second->m_DeviceGUI->redraw();
	}
}

//////////////////////////////////////////////////////////

void SynthModular::LoadPatch(const char *fn, const char *branchId)
{
	iostream *stream = NULL;
	ifstream file;
	fstream inf;
	std::stringstream packaged;
	std::string branch;
	std::auto_ptr<Spiral::File::Project> project;
	std::string sidecars;
	Spiral::File::DocumentSection document;

	if (Spiral::File::Project::PathLooksLikePackage(fn))
	{
		project.reset(new Spiral::File::Project(fn));
		std::string error;
		bool opened = (branchId && *branchId)
			? project->SwitchBranch(branchId, error)
			: project->OpenPackage("", error);

		if (!opened)
		{
			fl_message("%s", error.empty() ? "Error opening SSM Project" : error.c_str());

			return;
		}

		if (project->Source().Empty() && !project->HasPatch())
		{
			fl_message("This SSM Project has no patch in it.");

			return;
		}

		packaged.str(project->Source().Bytes());
		stream = &packaged;
		branch = project->GetIdentity().ActiveBranchID;
		m_SavePointCount = project->GetIdentity().Branches.size();
		sidecars = project->SidecarDirectory();
		document = project->GetDocument();
	}
	else
	{
		file.open(fn);
		if (!file)
			return;

		inf.open(fn, std::ios::in);
		stream = &inf;
		m_SavePointCount = 0;
	}

	const std::string json = project.get() && project->HasPatch() ? project->Patch().Bytes() : JSONDocument(*stream);
	Spiral::Core::PatchDescription patch;
	std::string error;
	if (!json.empty() && !Spiral::File::PatchFile::ReadText(json, patch, error))
	{
		fl_message("%s", error.c_str());
		return;
	}

	AudioTransportHub *hub = AudioTransportHub::Get();
	pthread_mutex_lock(&m_CycleLock);
	hub->BeginPatchReplacement();
	pthread_mutex_unlock(&m_CycleLock);

	m_FilePath=fn;
	m_BranchID=branch;
	ClearUp();
	m_Document = document;

	if (m_Drawer)
		m_Drawer->Refresh();

	if (!json.empty())
		Apply(patch, sidecars);
	else
		StreamPatchIn(*stream, false, sidecars);

	pthread_mutex_lock(&m_CycleLock);
	hub->EndPatchReplacement();
	pthread_mutex_unlock(&m_CycleLock);

	if (stream == &inf)
		inf.close();

	TITLEBAR=LABEL+" "+fn;
	m_TopWindow->label(TITLEBAR.c_str());
}

void SynthModular::SavePatch(const char *fn)
{
	if (Spiral::File::Project::PathLooksLikePackage(fn))
	{
		// A fresh project, then SaveAs. Constructing on the destination
		// would treat a not-yet-opened package as the current file.
		Spiral::File::Project project("");
		std::string error;

		if (!project.BeginSidecars(error))
		{
			fl_message("%s", error.c_str());

			return;
		}

		Spiral::Core::PatchDescription patch;
		std::string json;
		::Describe(patch, *this, project.SidecarDirectory());

		if (!Spiral::File::PatchFile::Text(patch, json, error))
		{
			fl_message("%s", error.c_str());

			return;
		}

		project.Patch().Set(json);
		project.SetDocument(m_Document);

		if (!project.SaveAs(fn, error))
		{
			fl_message("%s", error.empty() ? "Error saving SSM Project" : error.c_str());

			return;
		}
	}
	else
	{
		fl_message("Save As writes a project (.ssmp).");

		return;
	}

	m_FilePath = fn;
	m_BranchID.clear();
	TITLEBAR = LABEL + " " + fn;
	m_TopWindow->label(TITLEBAR.c_str());
}

static std::string MenuLabel(const std::string &name)
{
	std::string label;

	for (size_t c = 0; c < name.size(); ++c)
	{
		if (name[c] == '/' || name[c] == '\\' || name[c] == '&')
			label += '\\';

		label += name[c];
	}

	return label;
}

void SynthModular::SaveBranch(bool ask)
{
	if (!Spiral::File::Project::PathLooksLikePackage(m_FilePath))
	{
		fl_message("Save As an SSM Project before saving a branch.");

		return;
	}

	Spiral::File::Project project(m_FilePath);
	std::string error;
	bool opened = m_BranchID.empty()
		? project.OpenPackage("", error)
		: project.SwitchBranch(m_BranchID, error);

	if (!opened)
	{
		fl_message("%s", error.empty() ? "Error opening SSM Project" : error.c_str());

		return;
	}

	const Spumoni::Identity &identity = project.GetIdentity();
	int choice = 2;

	if (ask)
	{
		choice = fl_choice("Save Patch", "Cancel", "Create New Save Point", "Replace Current Save Point");
		if (choice == 0)
			return;
	}

	bool replace = choice != 1;
	std::string name;

	if (!replace)
	{
		std::string suggested = Spumoni::Project::SuggestedSavePointName(identity);
		const char *entered = fl_input("Save Point Name", suggested.c_str());

		if (!entered)
			return;

		name = entered;
		if (name.find_first_not_of(" \t\r\n") == std::string::npos)
		{
			fl_message("A new save point needs a name.");

			return;
		}
	}
	else if (!identity.ActiveBranchName.empty())
		name = identity.ActiveBranchName;
	else
		name = project.BranchNameFromPath(m_FilePath);

	if (!project.BeginSidecars(error))
	{
		fl_message("%s", error.c_str());

		return;
	}

	Spiral::Core::PatchDescription patch;
	std::string json;
	::Describe(patch, *this, project.SidecarDirectory());

	if (!Spiral::File::PatchFile::Text(patch, json, error))
	{
		fl_message("%s", error.c_str());

		return;
	}

	project.Patch().Set(json);
	project.SetDocument(m_Document);

	if (!project.CreateSavePoint(name, replace, error))
	{
		fl_message("%s", error.empty() ? "Error saving SSM Project" : error.c_str());

		return;
	}

	m_BranchID = project.GetIdentity().ActiveBranchID;
	m_SavePointCount = project.GetIdentity().Branches.size();
	TITLEBAR = LABEL + " " + m_FilePath;
	m_TopWindow->label(TITLEBAR.c_str());
}

inline void SynthModular::cb_ImportBranch_i (Fl_Widget *o, void *v)
{
	std::string error;

	if (!Spiral::File::Project::PathLooksLikePackage(m_FilePath))
	{
		fl_message("Save this patch as an SSM Project before importing a branch.");

		return;
	}

	char *fn = fl_file_chooser ("Import a patch as a branch",
		"SSM Patches (*.{ssm,ssmp})\tSSM Project (*.ssmp)\tLegacy SSM (*.ssm)", NULL);

	if (!fn || *fn=='\0')
		return;

	std::string imported;

	if (Spiral::File::Project::PathLooksLikePackage(fn))
	{
		Spiral::File::Project source(fn);

		if (!source.OpenPackage("", error) || (source.Source().Empty() && !source.HasPatch()))
		{
			fl_message("%s", error.empty() ? "Error opening SSM Project" : error.c_str());

			return;
		}

		imported = source.HasPatch() ? source.Patch().Bytes() : source.Source().Bytes();
	}
	else
	{
		ifstream in(fn);

		if (!in)
		{
			fl_message("Cannot read %s", fn);

			return;
		}

		std::ostringstream bytes;
		bytes << in.rdbuf();
		imported = bytes.str();
	}

	Spiral::File::Project project(m_FilePath);

	if (!project.OpenPackage("", error))
	{
		fl_message("%s", error.empty() ? "Error opening SSM Project" : error.c_str());

		return;
	}

	std::string name = project.BranchNameFromPath(fn);
	const char *entered = fl_input("Name for the imported branch:", name.c_str());

	if (!entered)
		return;

	name = entered;
	if (name.find_first_not_of(" \t\r\n") == std::string::npos)
	{
		fl_message("An imported branch needs a name.");

		return;
	}

	std::istringstream importedStream(imported);

	if (!JSONDocument(importedStream).empty())
		project.Patch().Set(imported);
	else
		project.Source().Set(imported);

	if (!project.CreateSavePoint(name, false, error, true))
	{
		fl_message("%s", error.empty() ? "Error importing branch" : error.c_str());

		return;
	}

	m_SavePointCount = project.GetIdentity().Branches.size();
}

void SynthModular::cb_ImportBranch(Fl_Widget* o, void* v)
{
	((SynthModular*)(v))->cb_ImportBranch_i(o,v);
}

inline void SynthModular::cb_SavePoints_i (Fl_Widget *o, void *v)
{
	if (!Spiral::File::Project::PathLooksLikePackage(m_FilePath))
	{
		fl_message("Open an SSM Project to browse save points.");

		return;
	}

	Spiral::File::Project project(m_FilePath);
	std::string error;

	if (!project.OpenPackage("", error))
	{
		fl_message("%s", error.empty() ? "Error opening SSM Project" : error.c_str());

		return;
	}

	const Spumoni::Identity identity = project.GetIdentity();

	if (identity.Branches.empty())
	{
		fl_message("Open an SSM Project to browse save points.");

		return;
	}

	std::string current = m_BranchID.empty() ? identity.ActiveBranchID : m_BranchID;
	Fl_Window dialog(550, 150, "Save Points");
	Fl_Box warning(10, 5, 530, 30, "Opening replaces unsaved changes in the current patch.");
	Fl_Choice points(20, 45, 510, 30);

	for (size_t i = 0; i < identity.Branches.size(); ++i)
	{
		std::string name = identity.Branches[i].Name;

		if (identity.Branches[i].ID == current)
			name += " (current)";

		points.add(MenuLabel(name).c_str());

		if (identity.Branches[i].ID == current)
			points.value((int)i);
	}

	Fl_Return_Button open(220, 100, 190, 30, "Open Save Point");
	Fl_Button cancel(420, 100, 110, 30, "Cancel");
	dialog.end();
	dialog.set_modal();
	dialog.show();
	bool accepted = false;

	while (dialog.shown())
	{
		Fl::wait();
		Fl_Widget *action;

		while ((action = Fl::readqueue()))
		{
			if (action == &open || action == &cancel || action == &dialog)
			{
				accepted = action == &open;
				dialog.hide();
			}
		}
	}

	if (!accepted || points.value() < 0)
		return;

	std::string id = identity.Branches[points.value()].ID;
	LoadPatch(m_FilePath.c_str(), id.c_str());
}

void SynthModular::cb_SavePoints (Fl_Widget *o, void *v)
{
	((SynthModular*)(o->user_data()))->cb_SavePoints_i (o, v);
}
