AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/KeyboardPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/panels/KeyboardPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = KeyboardPlugin_GUI.la
KeyboardPlugin_GUI_la_SOURCES = KeyboardPluginGUI.cpp KeyboardPluginGUI.h
KeyboardPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
KeyboardPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/KeyboardPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/KeyboardPlugin_GUI.so"
