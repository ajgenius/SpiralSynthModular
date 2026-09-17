AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/LADSPAPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/LADSPAPlugin
plugin_LTLIBRARIES = LADSPAPlugin_GUI.la
LADSPAPlugin_GUI_la_SOURCES = LADSPAPluginGUI.cpp LADSPAPluginGUI.h
LADSPAPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
LADSPAPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/LADSPAPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/LADSPAPlugin_GUI.so"
