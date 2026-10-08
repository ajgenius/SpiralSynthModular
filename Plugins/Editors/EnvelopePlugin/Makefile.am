AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/EnvelopePlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/EnvelopePlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = EnvelopePlugin_GUI.la
EnvelopePlugin_GUI_la_SOURCES = EnvelopePluginGUI.cpp EnvelopePluginGUI.h
EnvelopePlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
EnvelopePlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/EnvelopePlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/EnvelopePlugin_GUI.so"
