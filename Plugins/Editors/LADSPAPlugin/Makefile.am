AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/LADSPAPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/LADSPAPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = LADSPAPlugin_GUI.la
LADSPAPlugin_GUI_la_SOURCES = LADSPAPluginGUI.cpp LADSPAPluginGUI.h
LADSPAPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
LADSPAPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/LADSPAPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/LADSPAPlugin_GUI.so"
