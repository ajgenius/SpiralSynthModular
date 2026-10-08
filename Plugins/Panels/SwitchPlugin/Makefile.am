AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/SwitchPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/panels/SwitchPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = SwitchPlugin_GUI.la
SwitchPlugin_GUI_la_SOURCES = SwitchPluginGUI.cpp SwitchPluginGUI.h
SwitchPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
SwitchPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SwitchPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SwitchPlugin_GUI.so"
