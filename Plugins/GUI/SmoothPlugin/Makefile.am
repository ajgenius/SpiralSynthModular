AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/SmoothPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/SmoothPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = SmoothPlugin_GUI.la
SmoothPlugin_GUI_la_SOURCES = SmoothPluginGUI.cpp SmoothPluginGUI.h
SmoothPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
SmoothPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SmoothPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SmoothPlugin_GUI.so"
