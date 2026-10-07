AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/FlipflopPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = FlipflopPlugin_DSP.la
FlipflopPlugin_DSP_la_SOURCES = FlipflopPlugin.h FlipflopPlugin.C
FlipflopPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
FlipflopPlugin_DSP_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/FlipflopPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/FlipflopPlugin_DSP.so"
