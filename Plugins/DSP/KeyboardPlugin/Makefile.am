AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/dsp/KeyboardPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = KeyboardPlugin_DSP.la
KeyboardPlugin_DSP_la_SOURCES = KeyboardPlugin.h KeyboardPlugin.C
KeyboardPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
KeyboardPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/KeyboardPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/KeyboardPlugin_DSP.so"
