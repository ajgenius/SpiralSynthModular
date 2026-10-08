AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/NoisePlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = NoisePlugin_DSP.la
NoisePlugin_DSP_la_SOURCES = NoisePlugin.h NoisePlugin.C
NoisePlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
NoisePlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/NoisePlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/NoisePlugin_DSP.so"
