AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/Plugins/Devices/NoteSnapPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = @PLUGINDIR@/panels/NoteSnapPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = NoteSnapPlugin_GUI.la
NoteSnapPlugin_GUI_la_SOURCES = NoteSnapPluginGUI.cpp NoteSnapPluginGUI.h
NoteSnapPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
NoteSnapPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/libs/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/NoteSnapPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/NoteSnapPlugin_GUI.so"
