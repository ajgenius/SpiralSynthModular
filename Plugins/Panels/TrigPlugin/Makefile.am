AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/TrigPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/panels/TrigPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = TrigPlugin_GUI.la
TrigPlugin_GUI_la_SOURCES = TrigPluginGUI.cpp TrigPluginGUI.h
TrigPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
TrigPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/TrigPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/TrigPlugin_GUI.so"
