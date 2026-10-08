AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/WaveShaperPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/WaveShaperPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = WaveShaperPlugin_GUI.la
WaveShaperPlugin_GUI_la_SOURCES = WaveShaperPluginGUI.cpp WaveShaperPluginGUI.h
WaveShaperPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
WaveShaperPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/WaveShaperPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/WaveShaperPlugin_GUI.so"
