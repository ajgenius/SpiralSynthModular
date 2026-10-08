AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/WaveTablePlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/panels/WaveTablePlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = WaveTablePlugin_GUI.la
WaveTablePlugin_GUI_la_SOURCES = WaveTablePluginGUI.cpp WaveTablePluginGUI.h
WaveTablePlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
WaveTablePlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/WaveTablePlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/WaveTablePlugin_GUI.so"
