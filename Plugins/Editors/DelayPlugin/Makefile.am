AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/DelayPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/DelayPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = DelayPlugin_GUI.la
DelayPlugin_GUI_la_SOURCES = DelayPluginGUI.cpp DelayPluginGUI.h
DelayPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
DelayPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/DelayPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/DelayPlugin_GUI.so"
