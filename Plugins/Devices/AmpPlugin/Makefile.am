AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/AmpPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = AmpPlugin_DSP.la
AmpPlugin_DSP_la_SOURCES = AmpPlugin.h AmpPlugin.C
AmpPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
AmpPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/AmpPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/AmpPlugin_DSP.so"
