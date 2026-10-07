AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/DistributorPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/DistributorPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = DistributorPlugin_GUI.la
DistributorPlugin_GUI_la_SOURCES = DistributorPluginGUI.cpp DistributorPluginGUI.h
DistributorPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
DistributorPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/DistributorPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/DistributorPlugin_GUI.so"
