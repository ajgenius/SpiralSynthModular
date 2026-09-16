AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/OutputPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/OutputPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = OutputPlugin_GUI.la
OutputPlugin_GUI_la_SOURCES = OutputPluginGUI.cpp OutputPluginGUI.h
OutputPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
OutputPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/OutputPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/OutputPlugin_GUI.so"
