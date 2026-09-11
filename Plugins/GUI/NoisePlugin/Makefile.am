AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/NoisePlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = NoisePlugin_GUI.la
NoisePlugin_GUI_la_SOURCES = NoisePluginGUI.cpp NoisePluginGUI.h
NoisePlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
NoisePlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/NoisePlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/NoisePlugin_GUI.so"
