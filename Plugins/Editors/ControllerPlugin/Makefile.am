AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/ControllerPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/ControllerPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = ControllerPlugin_GUI.la
ControllerPlugin_GUI_la_SOURCES = ControllerPluginGUI.cpp ControllerPluginGUI.h
ControllerPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
ControllerPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/ControllerPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/ControllerPlugin_GUI.so"
