AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/EnvFollowerPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/EnvFollowerPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = EnvFollowerPlugin_GUI.la
EnvFollowerPlugin_GUI_la_SOURCES = EnvFollowerPluginGUI.cpp EnvFollowerPluginGUI.h
EnvFollowerPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
EnvFollowerPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/EnvFollowerPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/EnvFollowerPlugin_GUI.so"
