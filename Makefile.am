AUTOMAKE_OPTIONS = foreign
SUBDIRS = GUI SpiralSound

INCLUDES = $(FLTK_CXXFLAGS) $(FLTK_CFLAGS) -I./GUI/Widgets -I./SpiralSound -I./SpiralSound/Plugins
LIBS = $(FLTK_LIBS) -lm -ldl -lpthread

EXTRA_DIST = autogen.sh	
SpiralPlugins = $(libdir)/SpiralPlugins

bin_PROGRAMS = SpiralSynthModular
SpiralSynthModular_SOURCES = \
		GUI/Widgets/Fl_DeviceGUI.h \
		GUI/Widgets/Fl_DeviceGUI.C \
		GUI/Widgets/Fl_CommentGUI.h \
		GUI/Widgets/Fl_CommentGUI.C \
		GUI/Widgets/Fl_DragBar.H \
		GUI/Widgets/Fl_DragBar.cxx \
		GUI/Widgets/Fl_Canvas.h \
		GUI/Widgets/Fl_Canvas.C \
		GUI/Widgets/PawfalYesNo.h \
		GUI/Widgets/PawfalYesNo.C \
		GUI/Widgets/PawfalInput.h \
		GUI/Widgets/PawfalInput.C \
                GUI/Widgets/SpiralGUI.H \
                GUI/Widgets/SpiralGUI.C \
		SpiralSound/SpiralInfo.h \
		SpiralSound/SpiralInfo.C \
		SpiralSound/Sample.h \
		SpiralSound/Sample.C \
		SpiralSound/PluginManager.h \
		SpiralSound/PluginManager.C \
		SpiralSound/ChannelHandler.h \
		SpiralSound/ChannelHandler.C \
		SpiralSound/Plugins/SpiralPlugin.h \
		SpiralSound/Plugins/SpiralPlugin.C \
		SpiralSound/Plugins/SpiralPluginGUI.h \
		SpiralSound/Plugins/SpiralPluginGUI.C \
		SpiralSynthPluginLocation.h \
		SpiralSynthModular.h \
		SpiralSynthModular.C \
		SettingsWindow.h \
		SettingsWindow.C \
		GraphSort.h \
		GraphSort.C \
		main.cpp
