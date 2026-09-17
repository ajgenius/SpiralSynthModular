AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/FilterPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/FilterPlugin
plugin_LTLIBRARIES = FilterPlugin_GUI.la
FilterPlugin_GUI_la_SOURCES = FilterPluginGUI.cpp FilterPluginGUI.h
FilterPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
FilterPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/FilterPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/FilterPlugin_GUI.so"
