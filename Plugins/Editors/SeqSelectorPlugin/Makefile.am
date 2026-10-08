AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/SeqSelectorPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/gui/SeqSelectorPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = SeqSelectorPlugin_GUI.la
SeqSelectorPlugin_GUI_la_SOURCES = SeqSelectorPluginGUI.cpp SeqSelectorPluginGUI.h
SeqSelectorPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
SeqSelectorPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SeqSelectorPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SeqSelectorPlugin_GUI.so"
