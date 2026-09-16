AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/StereoMixerPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/StereoMixerPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = StereoMixerPlugin_GUI.la
StereoMixerPlugin_GUI_la_SOURCES = StereoMixerPluginGUI.cpp StereoMixerPluginGUI.h
StereoMixerPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
StereoMixerPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/StereoMixerPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/StereoMixerPlugin_GUI.so"
