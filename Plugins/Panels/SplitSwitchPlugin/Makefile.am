AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/SplitSwitchPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/panels/SplitSwitchPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = SplitSwitchPlugin_GUI.la
SplitSwitchPlugin_GUI_la_SOURCES = SplitSwitchPluginGUI.cpp SplitSwitchPluginGUI.h
SplitSwitchPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
SplitSwitchPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SplitSwitchPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SplitSwitchPlugin_GUI.so"
