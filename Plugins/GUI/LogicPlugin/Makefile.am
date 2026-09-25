AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/LogicPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/LogicPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = LogicPlugin_GUI.la
LogicPlugin_GUI_la_SOURCES = LogicPluginGUI.cpp LogicPluginGUI.h
LogicPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
LogicPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/LogicPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/LogicPlugin_GUI.so"
