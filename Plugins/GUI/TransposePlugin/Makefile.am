AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/TransposePlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = TransposePlugin_GUI.la
TransposePlugin_GUI_la_SOURCES = TransposePluginGUI.cpp TransposePluginGUI.h
TransposePlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
TransposePlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/TransposePlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/TransposePlugin_GUI.so"
