AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/LFOPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/LFOPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = LFOPlugin_GUI.la
LFOPlugin_GUI_la_SOURCES = LFOPluginGUI.cpp LFOPluginGUI.h
LFOPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
LFOPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/LFOPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/LFOPlugin_GUI.so"
