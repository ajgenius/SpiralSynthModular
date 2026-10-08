AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/CounterPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/panels/CounterPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = CounterPlugin_GUI.la
CounterPlugin_GUI_la_SOURCES = CounterPluginGUI.cpp CounterPluginGUI.h
CounterPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
CounterPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/CounterPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/CounterPlugin_GUI.so"
