AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir) -I$(top_srcdir)/Plugins/DSP/SpiralLoopPlugin $(FLTK_CFLAGS) $(FLTK_CXXFLAGS)
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/gui/SpiralLoopPlugin
plugin_LTLIBRARIES = SpiralLoopPlugin_GUI.la
SpiralLoopPlugin_GUI_la_SOURCES = SpiralLoopPluginGUI.cpp SpiralLoopPluginGUI.h
SpiralLoopPlugin_GUI_la_LDFLAGS = -module -avoid-version -shared
SpiralLoopPlugin_GUI_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralWidgets.la $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la $(FLTK_LIBS) -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SpiralLoopPlugin_GUI.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SpiralLoopPlugin_GUI.so"
