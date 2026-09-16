AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/NoteSnapPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/NoteSnapPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = NoteSnapPlugin_GUI.la
NoteSnapPlugin_GUI_la_SOURCES = NoteSnapPluginGUI.cpp NoteSnapPluginGUI.h
NoteSnapPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
NoteSnapPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/NoteSnapPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/NoteSnapPlugin_GUI.so"
