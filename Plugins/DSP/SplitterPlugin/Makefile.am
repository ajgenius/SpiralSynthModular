AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/SplitterPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = SplitterPlugin_DSP.la
SplitterPlugin_DSP_la_SOURCES = SplitterPlugin.h SplitterPlugin.C
SplitterPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SplitterPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SplitterPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SplitterPlugin_DSP.so"
