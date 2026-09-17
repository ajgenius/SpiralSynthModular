AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/EnvelopePlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/EnvelopePlugin
plugin_LTLIBRARIES = EnvelopePlugin_GUI.la
EnvelopePlugin_GUI_la_SOURCES = EnvelopePluginGUI.cpp EnvelopePluginGUI.h
EnvelopePlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
EnvelopePlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/EnvelopePlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/EnvelopePlugin_GUI.so"
