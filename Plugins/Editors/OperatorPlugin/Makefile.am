AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/OperatorPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/OperatorPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = OperatorPlugin_GUI.la
OperatorPlugin_GUI_la_SOURCES = OperatorPluginGUI.cpp OperatorPluginGUI.h
OperatorPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
OperatorPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/OperatorPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/OperatorPlugin_GUI.so"
