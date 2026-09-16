AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/MixerPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/MixerPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = MixerPlugin_GUI.la
MixerPlugin_GUI_la_SOURCES = MixerPluginGUI.cpp MixerPluginGUI.h
MixerPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
MixerPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/MixerPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/MixerPlugin_GUI.so"
