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
#include <FL/x.H>
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
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include "GUI/Widgets/Fl_DeviceGUI.h"
#include "GUI/Widgets/Fl_CommentGUI.h"
#include "GUI/Widgets/Fl_Canvas.h"
#include "SpiralSound/Plugins/SpiralPlugin.h"
#include "SpiralSound/ChannelHandler.h"
#include "SettingsWindow.h"

const static string VER_STRING = "0.2.3.cvs";

typedef Fl_Double_Window SpiralWindowType;
// typedef Fl_Window SpiralWindowType;

class SettingsWindow;

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
	DeviceGroup() { devicecount = 0; }

	int devicecount;
	fstream devices;
	map<int,int> m_DeviceIds;//old ID, new ID
};

class SynthModular
{
public:
	SynthModular();
	~SynthModular();

	SpiralWindowType * CreateWindow();
	void LoadPlugins(string pluginPath);
	void Update();
	void AddDevice(int n, int x, int y);
	void AddComment(int n);
	void ClearUp();
	void UpdateHostInfo();
	bool CallbackMode() { return m_CallbackUpdateMode; }
	bool IsBlockingOutputPluginReady() { return m_BlockingOutputPluginIsReady; }
	void UpdatePluginGUIs();
	void LoadPatch(const char *fn);

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
		m_Info.PAUSED = true;
	}

	void ResumeAudio()
	{
		m_Info.PAUSED = false;
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

	iostream &StreamPatchIn(iostream &s, bool paste, bool merge);
private:

	vector<string> BuildPluginList(const string &Path);

	DeviceWin* NewDeviceWin(int n, int x, int y);
	DeviceWin* NewComment(int n, int x, int y);

	HostInfo m_Info;
	bool m_ResetingAudioThread, m_HostNeedsUpdate, m_Frozen;


	static DeviceGUIInfo BuildDeviceGUIInfo(PluginInfo &PInfo);

	// currently static, to allow the callback
	// cb_UpdatePluginInfo to access the map.
	static map<int,DeviceWin*> m_DeviceWinMap;

        int m_NextID;
	static bool m_CallbackUpdateMode;
	static bool m_BlockingOutputPluginIsReady;
	string m_FilePath, m_MergeFilePath;

	// Main GUI stuff
	void CreateGUI (int xoff=0, int yoff=0, char *name="");
        Fl_Menu_Bar *m_MainMenu;
        Fl_Pack *m_Topbar, *m_ToolbarPanel, *m_Toolbar;
        Fl_Group *m_GroupFiller;
	Fl_Button *m_Load, *m_Save, *m_New, *m_Options, *m_NewComment;
	Fl_Pack *m_PlayResetGroup;
	Fl_Button *m_PlayPause, *m_Reset;
        Fl_Tabs         *m_GroupTab;
	Fl_Canvas 	*m_Canvas;
	Fl_Scroll	*m_CanvasScroll;
	map<string,Fl_Pack*> m_PluginGroupMap;
	SettingsWindow  *m_SettingsWindow;
	SpiralWindowType* m_TopWindow;
	vector<Fl_Button*> m_DeviceVec;
	ChannelHandler m_CH; // used for threadsafe communication
	DeviceGroup	m_Copied;

	inline void cb_NewComment_i(Fl_Button* o, void* v);
	static void cb_NewComment(Fl_Button* o, void* v);
        // File menu - and associated buttons, etc.
        inline void cb_New_i (Fl_Widget *o, void *v);
	static void cb_New (Fl_Widget *o, void *v);
	inline void cb_Load_i (Fl_Widget *o, void *v);
	static void cb_Load (Fl_Widget *o, void *v);
	inline void cb_Save_i (Fl_Widget *o, void *v);
	static void cb_Save (Fl_Widget *o, void *v);
	inline void cb_Merge_i (Fl_Widget *o, void *v);
	static void cb_Merge (Fl_Widget *o, void *v);
	inline void cb_Close_i (Fl_Widget *o, void *v);
	static void cb_Close (Fl_Widget *o, void *v);
        // Edit menu
	inline void cb_Cut_i (Fl_Widget *o, void *v);
	static void cb_Cut (Fl_Widget *o, void *v);
	inline void cb_Copy_i (Fl_Widget *o, void *v);
	static void cb_Copy (Fl_Widget *o, void *v);
	inline void cb_Paste_i (Fl_Widget *o, void *v);
	static void cb_Paste (Fl_Widget *o, void *v);
	inline void cb_Delete_i (Fl_Widget *o, void *v);
	static void cb_Delete (Fl_Widget *o, void *v);
        inline void cb_Options_i (Fl_Widget *o, void *v);
	static void cb_Options (Fl_Widget *o, void *v);
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
	inline void cb_GroupTab_i(Fl_Tabs* o, void* v);
	static void cb_GroupTab(Fl_Tabs* o, void* v);
	static void cb_Update(void* o, bool Mode);
	static void cb_Blocking(void* o, bool Mode);
	static void cb_UpdatePluginInfo(int ID, void *PluginInfo);
        inline void cb_ChangeBufferAndSampleRate_i(long int NewBufferSize, long int NewSamplerate);
	static void cb_ChangeBufferAndSampleRate(long unsigned int NewBufferSize, long unsigned int NewSamplerate, void *o)
	{
		((SynthModular*)o)->cb_ChangeBufferAndSampleRate_i(NewBufferSize, NewSamplerate);
	}
	friend istream &operator>>(istream &s, SynthModular &o);
	friend ostream &operator<<(ostream &s, SynthModular &o);
};

iostream &operator>>(iostream &s, SynthModular &o);
ostream &operator<<(ostream &s, SynthModular &o);

#endif
