AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = TranslatePlugin_DSP.la
TranslatePlugin_DSP_la_SOURCES = TranslatePlugin.h TranslatePlugin.C
TranslatePlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
TranslatePlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/TranslatePlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/TranslatePlugin_DSP.so"
