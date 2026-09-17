AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/CounterPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/CounterPlugin
plugin_LTLIBRARIES = CounterPlugin_GUI.la
CounterPlugin_GUI_la_SOURCES = CounterPluginGUI.cpp CounterPluginGUI.h
CounterPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
CounterPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/CounterPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/CounterPlugin_GUI.so"
