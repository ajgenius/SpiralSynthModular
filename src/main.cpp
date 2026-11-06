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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
#include <cstring>
#include <FL/Fl.H>
#include <FL/Fl_Tooltip.H>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "SpiralSynthModular.h"
#include "SpiralInfo.h"
#include "MacBundle.h"
#ifdef __APPLE__
#include <objc/message.h>
#include <objc/runtime.h>
#include <FL/x.H>
#include <vector>
#endif

pthread_t loopthread,watchdogthread;
SynthModular *synth;
static pthread_mutex_t audioStopMutex = PTHREAD_MUTEX_INITIALIZER;
static bool audioStopRequested = false;

static bool AudioStopRequested()
{
	pthread_mutex_lock(&audioStopMutex);
	bool stop = audioStopRequested;
	pthread_mutex_unlock(&audioStopMutex);

	return stop;
}

char watchdog_check = 1;
char gui_watchdog_check = 1;

int pthread_create_realtime (pthread_t *new_thread,
			 void *(*start)(void *), void *arg,
			 int priority);

bool CallbackOnly = false;
bool FIFO = false;
bool GUI = true;

/////////////////////////////////////////////////////////////

void *watchdog (void *arg)
{
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	watchdog_check = 0;
	gui_watchdog_check = 0;

	while (1)
	{
		usleep (10000000);

		// gui watchdog goes off with modal dialog boxes
		if (watchdog_check == 0)// || gui_watchdog_check== 0)
		{
			cerr<<"ssm watchdog: timeout - killing ssm"<<endl;

			if (watchdog_check==0)
				cerr<<"diagnosis: audio hung?"<<endl;

			if (gui_watchdog_check==0)
				cerr<<"diagnosis: gui starved by audio"<<endl;

			exit (1);
		}

		watchdog_check = 0;
		gui_watchdog_check = 0;
	}
}

///////////////////////////////////////////////////////////////////////

void *audioloop(void* o)
{
	while(!AudioStopRequested())
	{
		synth->Update();
		watchdog_check = 1;
	}

	return NULL;
}

//////////////////////////////////////////////////////
#include "GraphSort.h"

#ifdef __APPLE__
static vector<string> s_Handed;

static void OpenHanded(void *)
{
	static bool opening = false;

	if (opening || !synth)
		return;

	opening = true;

	while (!s_Handed.empty())
	{
		const string path = s_Handed.front();
		s_Handed.erase(s_Handed.begin());
		synth->OpenDropped(path);
	}

	opening = false;
}

static void Handed(const char *path)
{
	if (!path || !*path)
		return;

	s_Handed.push_back(path);
	Fl::add_timeout(0.0, OpenHanded);
}
#endif

int main(int argc, char **argv)
{
	srand(time(NULL));
	SpiralInfo::Get()->LoadPrefs();

	// get args
	string cmd_filename="";
	bool cmd_specd = false;
	string cmd_saveAs="";
	string cmd_pluginPath=SpiralBundlePluginPath();

	// parse the args
	if (argc>1)
	{
		for (int a=1; a<argc; a++)
		{
#ifdef __APPLE__
			if (!strncmp(argv[a], "-psn_", 5))
				continue;
#endif

			if (!strcmp(argv[a],"--NoGUI"))
				GUI = false;
			else if (!strcmp(argv[a],"--Realtime"))
				FIFO = true;
			else if (!strcmp(argv[a],"-h"))
			{
				cout<<"usage: spiralsynthmodular [patch.ssm|patch.ssmp] [options]"<<endl<<endl
				<<"options list"<<endl
				<<"-h : help"<<endl
				<<"-v : print version"<<endl
				<<"--NoGUI : run without GUI (only useful when loading patch from command line"<<endl
				<<"--Realtime : spawn audio thread with FIFO scheduling (run as root)"<<endl
				<<"--PluginPath <PATH> : look for plugins in the specified directory"<<endl
				<<"--SaveAs <FILE> : save the loaded patch to FILE and exit (with --NoGUI, a headless re-save)"<<endl;
				exit(0);
			}
			else if (!strcmp(argv[a],"-v"))
			{
				cout<<VER_STRING<<endl;
				exit(0);
			}
			else if (!strcmp(argv[a],"--PluginPath"))
			{
				a++;
				cmd_pluginPath=argv[a];
			}
			else if (!strcmp(argv[a],"--SaveAs") && a+1<argc)
			{
				a++;
				cmd_saveAs=argv[a];
			}
			else
			{
				cmd_filename = argv[a];
				cmd_specd = true;
			}
		}
	}

	// set some fltk defaults
	Fl_Tooltip::size(10);
	Fl::visible_focus(false);
	Fl::visual(FL_DOUBLE|FL_RGB);

#ifdef __APPLE__
	((void (*)(id, SEL, BOOL))objc_msgSend)((id)objc_getClass("NSWindow"),
		sel_registerName("setAllowsAutomaticWindowTabbing:"), NO);
#endif

	synth=new SynthModular;

	// setup the synth
	synth->WantChrome(GUI);
	Fl_Window* win = synth->CreateWindow();

	{
		string root = cmd_pluginPath.empty() ? SpiralInfo::PLUGIN_PATH : cmd_pluginPath;

		if (!root.empty())
		{
			if (root[root.size()-1] != '/')
				root += '/';

			const string bundled = root + "ladspa";
			struct stat info;

			if (stat(bundled.c_str(), &info) == 0 && S_ISDIR(info.st_mode))
			{
				const char *existing = getenv("LADSPA_PATH");
				const string search = existing && *existing
				                    ? bundled + ":" + existing
				                    : bundled + ":/usr/lib/ladspa:/usr/local/lib/ladspa";
				setenv("LADSPA_PATH", search.c_str(), 1);
			}
		}
	}

	synth->LoadPlugins(cmd_pluginPath);
#ifdef __APPLE__
	if (GUI)
		fl_open_callback(Handed);
#endif
	win->xclass("");

	if (GUI)
		win->show(1, argv); // prevents stuff happening before the plugins have loaded

	// spawn the audio thread
	bool watchdogStarted = false;
	int audioError;

	if (FIFO)
	{
		watchdogStarted = (pthread_create_realtime(&watchdogthread,watchdog,NULL,sched_get_priority_max(SCHED_FIFO)) == 0);
		audioError = pthread_create_realtime(&loopthread,audioloop,NULL,sched_get_priority_max(SCHED_FIFO)-1);
	}
	else
	{
		audioError = pthread_create(&loopthread,NULL,audioloop,NULL);

		// reduce the priority of the gui
		if (setpriority(PRIO_PROCESS,0,20))
			cerr<<"Could not set priority for GUI thread"<<endl;
	}

	if (audioError != 0)
	{
		cerr << "Cannot start audio thread" << endl;

		if (watchdogStarted)
		{
			pthread_cancel(watchdogthread);
			pthread_join(watchdogthread, NULL);
		}

		delete synth;

		return 1;
	}

	// do we need to load a patch on startup?
	if (cmd_specd)
		synth->LoadPatch(cmd_filename.c_str());

	if (!cmd_saveAs.empty())
	{
		Fl::check();
		synth->SavePatch(cmd_saveAs.c_str());
		synth->FreezeAll();
		pthread_mutex_lock(&audioStopMutex);
		audioStopRequested = true;
		pthread_mutex_unlock(&audioStopMutex);
		pthread_join(loopthread, NULL);
		delete synth;

		return 0;
	}

	if (!GUI)
	{
		// if there is no gui needed, there is nothing more to do here
		Fl::check();

		for (;;)
			sleep(1);
	}

	for (;;)
	{
		if (!Fl::check())
			break;

		synth->UpdatePluginGUIs(); // deletes any if necc
		synth->RefreshMenuState();
		usleep(10000);
		gui_watchdog_check=1;
	}

	if (watchdogStarted)
	{
		pthread_cancel(watchdogthread);
		pthread_join(watchdogthread, NULL);
	}

	synth->FreezeAll();
	pthread_mutex_lock(&audioStopMutex);
	audioStopRequested = true;
	pthread_mutex_unlock(&audioStopMutex);
	pthread_join(loopthread, NULL);
	delete synth;

	return 0;
}

// nicked from Paul Barton-Davis' Ardour code :)
int pthread_create_realtime (pthread_t *new_thread, void *(*start)(void *), void *arg, int priority)
{
	pthread_attr_t *rt_attributes = (pthread_attr_t *) malloc (sizeof (pthread_attr_t));
	struct sched_param *rt_param = (struct sched_param *) malloc (sizeof (struct sched_param));

	pthread_attr_init (rt_attributes);

	if (seteuid (0))
	{
		cerr << "Cannot obtain root UID for RT scheduling operations"
		     << endl;

		return -1;
	}
	else
	{
		if (pthread_attr_setschedpolicy (rt_attributes, SCHED_FIFO))
		{
			cerr << "Cannot set FIFO scheduling attributes for RT thread" << endl;
		}

		if (pthread_attr_setscope (rt_attributes, PTHREAD_SCOPE_SYSTEM))
		{
			cerr << "Cannot set scheduling scope for RT thread" << endl;
		}

		rt_param->sched_priority = priority;

		if (pthread_attr_setschedparam (rt_attributes, rt_param))
		{
			cerr << "Cannot set scheduling priority for RT thread" << endl;
		}
	}

	int retval = pthread_create (new_thread, rt_attributes, start, arg);
	seteuid (getuid());

	return retval;
}
