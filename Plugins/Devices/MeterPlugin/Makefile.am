AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/MeterPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = MeterPlugin_DSP.la
MeterPlugin_DSP_la_SOURCES = MeterPlugin.h MeterPlugin.C
MeterPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
MeterPlugin_DSP_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/MeterPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/MeterPlugin_DSP.so"
