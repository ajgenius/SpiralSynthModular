AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/FormantFilterPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/FormantFilterPlugin
plugin_LTLIBRARIES = FormantFilterPlugin_GUI.la
FormantFilterPlugin_GUI_la_SOURCES = FormantFilterPluginGUI.cpp FormantFilterPluginGUI.h
FormantFilterPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
FormantFilterPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/FormantFilterPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/FormantFilterPlugin_GUI.so"
