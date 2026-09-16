AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/AmpPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/AmpPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = AmpPlugin_GUI.la
AmpPlugin_GUI_la_SOURCES = AmpPluginGUI.cpp AmpPluginGUI.h
AmpPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
AmpPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/AmpPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/AmpPlugin_GUI.so"
