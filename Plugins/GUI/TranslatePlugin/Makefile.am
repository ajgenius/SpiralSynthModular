AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/TranslatePlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/TranslatePlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = TranslatePlugin_GUI.la
TranslatePlugin_GUI_la_SOURCES = TranslatePluginGUI.cpp TranslatePluginGUI.h
TranslatePlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
TranslatePlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/TranslatePlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/TranslatePlugin_GUI.so"
