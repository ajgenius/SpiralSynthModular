AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/XFadePlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/XFadePlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = XFadePlugin_GUI.la
XFadePlugin_GUI_la_SOURCES = XFadePluginGUI.cpp XFadePluginGUI.h
XFadePlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
XFadePlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/XFadePlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/XFadePlugin_GUI.so"
