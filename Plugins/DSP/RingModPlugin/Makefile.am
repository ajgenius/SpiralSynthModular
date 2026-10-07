AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/RingModPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = RingModPlugin_DSP.la
RingModPlugin_DSP_la_SOURCES = RingModPlugin.h RingModPlugin.C
RingModPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
RingModPlugin_DSP_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/RingModPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/RingModPlugin_DSP.so"
