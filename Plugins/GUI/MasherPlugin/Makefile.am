AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/MasherPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = MasherPlugin_GUI.la
MasherPlugin_GUI_la_SOURCES = MasherPluginGUI.cpp MasherPluginGUI.h
MasherPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
MasherPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/MasherPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/MasherPlugin_GUI.so"
