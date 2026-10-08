AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/SVFilterPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = SVFilterPlugin_DSP.la
SVFilterPlugin_DSP_la_SOURCES = SVFilterPlugin.h SVFilterPlugin.C
SVFilterPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SVFilterPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SVFilterPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SVFilterPlugin_DSP.so"
