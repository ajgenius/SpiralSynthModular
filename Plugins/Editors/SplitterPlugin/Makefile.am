AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/SplitterPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/SplitterPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = SplitterPlugin_GUI.la
SplitterPlugin_GUI_la_SOURCES = SplitterPluginGUI.cpp SplitterPluginGUI.h
SplitterPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
SplitterPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SplitterPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SplitterPlugin_GUI.so"
