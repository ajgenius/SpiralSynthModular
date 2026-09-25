AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/MoogFilterPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/MoogFilterPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = MoogFilterPlugin_GUI.la
MoogFilterPlugin_GUI_la_SOURCES = MoogFilterPluginGUI.cpp MoogFilterPluginGUI.h
MoogFilterPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
MoogFilterPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/MoogFilterPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/MoogFilterPlugin_GUI.so"
