AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/SplitterPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = SplitterPlugin_DSP.la
SplitterPlugin_DSP_la_SOURCES = SplitterPlugin.h SplitterPlugin.C
SplitterPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SplitterPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SplitterPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SplitterPlugin_DSP.so"
