AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/PoshSamplerPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/PoshSamplerPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = PoshSamplerPlugin_GUI.la
PoshSamplerPlugin_GUI_la_SOURCES = PoshSamplerPluginGUI.cpp PoshSamplerPluginGUI.h
PoshSamplerPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
PoshSamplerPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/PoshSamplerPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/PoshSamplerPlugin_GUI.so"
