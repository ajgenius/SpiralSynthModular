AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/OscillatorPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/OscillatorPlugin
plugin_LTLIBRARIES = OscillatorPlugin_GUI.la
OscillatorPlugin_GUI_la_SOURCES = OscillatorPluginGUI.cpp OscillatorPluginGUI.h
OscillatorPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
OscillatorPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/OscillatorPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/OscillatorPlugin_GUI.so"
