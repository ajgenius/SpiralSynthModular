#include <pthread.h>
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

#ifndef SPIRALSYNTHMODULAR
#define SPIRALSYNTHMODULAR

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include "Fl_DeviceGUI.h"
#include "Fl_CommentGUI.h"
#include "Fl_Canvas.h"
#include "Fl_BoundaryDrawer.h"
#include "Fl_Breadcrumb.h"
#include "Fl_MidiBindings.h"
#include "SpiralPlugin.h"
#include "PatchDescription.h"
#include "MidiMap.h"
#include "ChannelHandler.h"
using Spiral::Core::ChannelHandler;
#include "SettingsWindow.h"
#include "config.h"

const static string VER_STRING = PACKAGE_VERSION;

class SpiralWindow : public Fl_Double_Window
{
public:
	SpiralWindow(int w, int h, const char *label)
		: Fl_Double_Window(w, h, label), Relayout(NULL), RelayoutData(NULL) {}

	void resize(int X, int Y, int W, int H)
	{
		Fl_Double_Window::resize(X, Y, W, H);

		if (Relayout)
			Relayout(RelayoutData);
	}

	void (*Relayout)(void *);
	void *RelayoutData;
};

typedef SpiralWindow SpiralWindowType;
// typedef Fl_Window SpiralWindowType;

class SettingsWindow;

namespace Spiral
{
	namespace Panels
	{
		class PanelControl;
	}
}

class DeviceWin
{
public:
	DeviceWin() : m_DeviceGUI(NULL) {}
	~DeviceWin();

	int 		  m_PluginID;
	SpiralPlugin* m_Device;
	Fl_DeviceGUI* m_DeviceGUI;
};

class Fl_ToolButton : public Fl_Button
{
public:
	Fl_ToolButton(int x, int y, int w, int h, const char *n=NULL) :
		Fl_Button(x,y,w,h,n) {}
	virtual void draw() { draw_label(); }
};

class DeviceGroup
{
public:
	map<int,int> m_DeviceIds;//old ID, new ID
};

class SynthModular
{
public:
	SynthModular();
	~SynthModular();

	void WantChrome(bool want) { m_WantChrome = want; }
	SpiralWindowType * CreateWindow();
	void LoadPlugins(string pluginPath);
	void Update();
	void AddDevice(int n, int x, int y);
	void AddComment(int n);
	void ClearUp(bool synchronize = true);
	void UpdateHostInfo();
	void UpdatePluginGUIs();
	void ApplyViewOptions();
	void LayoutChrome();
	void LoadPatch(const char *fn, const char *branchId = NULL);
	void MergePatch(const char *fn);
	void OpenDropped(const string &path);
	void Apply(const Spiral::Core::PatchDescription &patch, const string &sidecars, bool merge = false);
	bool ApplyPage(const Spiral::Core::PatchDescription &patch, const string &sidecars, bool merge, int page,
		std::map<std::string, int> &ids);
	DeviceWin* NewSubPatch(int x, int y, const string &name, SubPatchDevice *device,
		const Spiral::Core::SubPatchConfig &config);
	void GroupSelection(const string &title);
	void UngroupSelection();
	void PasteCopied(bool grouped);
	void AddEventInput();
	void RebuildVoices();
	void ClearVoices(bool keepVoiceOutputs = false);
	void SyncVoices();
	void SyncSubPatches();
	VoicePool *PoolFor(int id, long &voices);
	static SpiralPlugin *cb_FindDevice(int id, void *host);
	void SavePatch(const char *fn);

	void StopAudioForExit();

	void FreezeAll()
	{
		m_CH.Set("Frozen",true);
		m_CH.Wait();
	}

	void ThawAll()
	{
		m_CH.Set("Frozen",false);
	}

	void PauseAudio()
	{
		pthread_mutex_lock(&m_CycleLock);
		m_Info.PAUSED = true;
		pthread_mutex_unlock(&m_CycleLock);
	}

	void ResumeAudio()
	{
		pthread_mutex_lock(&m_CycleLock);
		m_Info.PAUSED = false;
		pthread_mutex_unlock(&m_CycleLock);
	}

	void ResetAudio()
	{
		if (! m_ResetingAudioThread)
		{
			FreezeAll();

			m_ResetingAudioThread = true;

			ThawAll();
		}
	}

	// only for audio thread
	bool IsFrozen() { return m_Frozen; }

	iostream &StreamPatchIn(iostream &s, bool merge, const string &sidecars = string());
	void RefreshMenuState();

	Spiral::Core::MidiMap &MidiMap() { return m_MidiMap; }
	Spiral::Panels::PanelControl *MidiControl(const string &device, const string &control);
	void LearnMidi(void (*learned)(int channel, int controller, void *data), void *data);
	void ServiceMidiInput();

private:
	DeviceWin* NewDeviceWin(int n, int x, int y);
	DeviceWin* NewComment(int n, int x, int y);

	void RenderAudio();
	void RunDevice(int id);
	static void cb_RunDevice(int id, void *host);
	bool m_RunVoices;
	pthread_mutex_t m_CycleLock;
	HostInfo m_Info;
	bool m_ResetingAudioThread, m_Frozen;
	int m_DriverFrames, m_DriverRate;

	static DeviceGUIInfo BuildDeviceGUIInfo(PluginInfo &PInfo);

	// currently static, to allow the callback
	// cb_UpdatePluginInfo to access the map.
	static map<int,DeviceWin*> m_DeviceWinMap;

	int m_NextID;
	map<int, VoicePool *> m_VoicePools;
	unsigned m_VoiceTopology;
	string m_Sidecars;
	unsigned m_PatchVoices;
	VoicePool m_PatchPool;
	bool m_VoicesStale;
	void SetPatchVoices(unsigned voices);
	static void cb_PatchVoices(unsigned voices, void *data);
	static bool m_CallbackUpdateMode;
	string m_FilePath, m_MergeFilePath, m_BranchID;
	unsigned m_SavePointCount;
	Spiral::File::DocumentSection m_Document;
	Spiral::Core::MidiMap m_MidiMap;
	void (*m_MidiLearn)(int, int, void *);
	void *m_MidiLearnData;
	Fl_MidiBindings *m_MidiPage;
	int m_PickDevice;
	bool m_PickOpened;
	void PickMidiDevice();
	void PickMidiControl(int id);
	void StopMidiPick();
	std::map<std::string, int> m_AppliedIds;
	bool m_MenuStateKnown, m_MenuSelection, m_MenuPaste, m_MenuSavePoints;
	bool m_MenuPackage;

	// Main GUI stuff
	void CreateGUI (int xoff=0, int yoff=0, const char *name="");
	void MenuAdd(const string &path, Fl_Callback *cb, void *data, int flags);
	void MenuRelabel(const string &oldLabel, const string &newLabel);
	void MenuEnable(const string &label, bool on);
	void MenuDropPlaceholders();
	Fl_Menu_Bar *m_MainMenu;
	Fl_Sys_Menu_Bar *m_SysMenu;
	bool             m_WantChrome;
	Fl_Pack *m_Topbar, *m_ToolbarPanel, *m_Toolbar;
	Fl_Group *m_GroupFiller;
	Fl_Button *m_Load, *m_Save, *m_New, *m_Options, *m_NewComment;
	Fl_Pack *m_PlayResetGroup;
	Fl_Button *m_PlayPause, *m_Reset;
	Fl_Tabs         *m_GroupTab;
	Fl_Canvas 	*m_Canvas;
	Fl_Scroll	*m_CanvasScroll;
	Fl_BoundaryDrawer *m_Drawer;
	Fl_Breadcrumb *m_Breadcrumb;
	map<string,Fl_Pack*> m_PluginGroupMap;
	SettingsWindow  *m_SettingsWindow;
	SpiralWindowType* m_TopWindow;
	vector<Fl_Button*> m_DeviceVec;
	ChannelHandler m_CH; // used for threadsafe communication
	DeviceGroup	m_Copied;
	Spiral::Core::PatchDescription m_CopiedPatch;
	string m_CopiedFrom;

	inline void cb_NewComment_i(Fl_Button* o, void* v);
	static void cb_NewComment(Fl_Button* o, void* v);
	// File menu - and associated buttons, etc.
	inline void cb_New_i (Fl_Widget *o, void *v);
	static void cb_New (Fl_Widget *o, void *v);
	inline void cb_Load_i (Fl_Widget *o, void *v);
	static void cb_Load (Fl_Widget *o, void *v);
	void ChooseAndLoadPatch(const char *directory);
	static void cb_Examples(Fl_Widget *o, void *v);
	inline void cb_Save_i (Fl_Widget *o, void *v);
	static void cb_Save (Fl_Widget *o, void *v);
	inline void cb_SaveAs_i (Fl_Widget *o, void *v);
	static void cb_SaveAs (Fl_Widget *o, void *v);
	void SaveBranch(bool ask = true);
	inline void cb_SavePoints_i (Fl_Widget *o, void *v);
	static void cb_SavePoints (Fl_Widget *o, void *v);
	inline void cb_Merge_i (Fl_Widget *o, void *v);
	static void cb_Merge (Fl_Widget *o, void *v);
	static void cb_MergeAsSubPatch (Fl_Widget *o, void *v);
	inline void cb_ImportBranch_i (Fl_Widget *o, void *v);
	static void cb_ImportBranch (Fl_Widget *o, void *v);
	inline void cb_Close_i (Fl_Widget *o, void *v);
	static void cb_Close (Fl_Widget *o, void *v);
	// Edit menu
	inline void cb_Cut_i (Fl_Widget *o, void *v);
	static void cb_Cut (Fl_Widget *o, void *v);
	inline void cb_Copy_i (Fl_Widget *o, void *v);
	static void cb_Copy (Fl_Widget *o, void *v);
	inline void cb_Paste_i (Fl_Widget *o, void *v);
	static void cb_Paste (Fl_Widget *o, void *v);
	static void cb_PasteAsSubPatch (Fl_Widget *o, void *v);
	inline void cb_Delete_i (Fl_Widget *o, void *v);
	static void cb_Delete (Fl_Widget *o, void *v);
	inline void cb_Options_i (Fl_Widget *o, void *v);
	static void cb_Options (Fl_Widget *o, void *v);
	static void cb_Relayout (void *v);
	static void cb_DrawerCollapse (void *v);
	static void cb_AddEvents (void *v);
	inline void cb_AboutPatch_i (Fl_Widget *o, void *v);
	static void cb_AboutPatch (Fl_Widget *o, void *v);
	inline void cb_MidiBindings_i (Fl_Widget *o, void *v);
	static void cb_MidiBindings (Fl_Widget *o, void *v);
	static std::string cb_MidiDeviceName (const std::string &device, void *v);
	static void cb_PickMidiDevice (void *v);
	static void cb_PickMidiControl (const std::string &device, void *v);
	static void cb_LearnMidi (void (*learned)(int, int, void *), void *learnData, void *v);
	static void cb_MidiSource (const std::string &source, void *v);
	static bool cb_DeviceAt (Fl_Canvas *canvas, int x, int y, Fl_Canvas::Pick &out, void *v);
	static void cb_DevicePicked (Fl_Canvas *canvas, const Fl_Canvas::Pick &pick, void *v);
	static bool cb_ControlAt (Fl_Canvas *canvas, int x, int y, Fl_Canvas::Pick &out, void *v);
	static void cb_ControlPicked (Fl_Canvas *canvas, const Fl_Canvas::Pick &pick, void *v);
	static void cb_Group (Fl_Widget *o, void *v);
	static void cb_Ungroup (Fl_Widget *o, void *v);
	static void cb_PageShown (Fl_Widget *o, void *v);
	inline void cb_About_i (Fl_Widget *o, void *v);
	static void cb_About (Fl_Widget *o, void *v);
	static void cb_CloseAbout (Fl_Widget *o, void *v);
	inline void cb_CenterPatch_i (Fl_Widget *o, void *v);
	static void cb_CenterPatch (Fl_Widget *o, void *v);
	// Plugin Menu
	inline void cb_NewDevice_i (Fl_Button *o, void *v);
	static void cb_NewDevice (Fl_Button *o, void *v);
	inline void cb_NewDeviceFromMenu_i (Fl_Widget *o, void *v);
	static void cb_NewDeviceFromMenu (Fl_Widget *o, void *v);
	inline void cb_NewDeviceFromCanvasMenu_i (Fl_Canvas *o, void *v);
	static void cb_NewDeviceFromCanvasMenu (Fl_Canvas *o, void *v);
	// Audio Menu
	inline void cb_PlayPause_i (Fl_Widget *o, void *v);
	static void cb_PlayPause (Fl_Widget *o, void *v);
	inline void cb_Reset_i (Fl_Widget *o, void *v);
	static void cb_Reset (Fl_Widget *o, void *v);

	inline void cb_Connection_i(Fl_Canvas* o, void* v);
	static void cb_Connection(Fl_Canvas* o, void* v);
	inline void cb_Unconnect_i(Fl_Canvas* o, void* v);
	static void cb_Unconnect(Fl_Canvas* o, void* v);
	static void cb_DropFile(Fl_Canvas* o, void* v);
	inline void cb_GroupTab_i(Fl_Tabs* o, void* v);
	static void cb_GroupTab(Fl_Tabs* o, void* v);
	static void cb_Update(void* o, bool Mode);
	static void cb_UpdatePluginInfo(int ID, void *PluginInfo);
	void cb_ChangeBufferAndSampleRate_i(long int NewBufferSize, long int NewSamplerate);
	static void cb_ChangeBufferAndSampleRate(long unsigned int NewBufferSize, long unsigned int NewSamplerate, void *o)
	{
		((SynthModular*)o)->cb_ChangeBufferAndSampleRate_i(NewBufferSize, NewSamplerate);
	}
	friend istream &operator>>(istream &s, SynthModular &o);
	friend void Describe(Spiral::Core::PatchDescription &patch, SynthModular &o, const string &sidecars);
	friend void DescribePage(Spiral::Core::PatchDescription &patch, SynthModular &o, const string &sidecars, int page,
		bool &external, const std::set<int> *only);
};

iostream &operator>>(iostream &s, SynthModular &o);

#endif
