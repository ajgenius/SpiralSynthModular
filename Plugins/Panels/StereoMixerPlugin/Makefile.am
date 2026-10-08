AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/StereoMixerPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/panels/StereoMixerPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = StereoMixerPlugin_GUI.la
StereoMixerPlugin_GUI_la_SOURCES = StereoMixerPluginGUI.cpp StereoMixerPluginGUI.h
StereoMixerPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
StereoMixerPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/StereoMixerPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/StereoMixerPlugin_GUI.so"
