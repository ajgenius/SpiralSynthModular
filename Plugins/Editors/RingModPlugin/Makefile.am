AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/RingModPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/RingModPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = RingModPlugin_GUI.la
RingModPlugin_GUI_la_SOURCES = RingModPluginGUI.cpp RingModPluginGUI.h
RingModPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
RingModPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/RingModPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/RingModPlugin_GUI.so"
