AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/AnotherFilterPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/AnotherFilterPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = AnotherFilterPlugin_GUI.la
AnotherFilterPlugin_GUI_la_SOURCES = AnotherFilterPluginGUI.cpp AnotherFilterPluginGUI.h
AnotherFilterPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
AnotherFilterPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/AnotherFilterPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/AnotherFilterPlugin_GUI.so"
