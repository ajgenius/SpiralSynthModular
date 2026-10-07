AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/BeatMatchPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = BeatMatchPlugin_DSP.la
BeatMatchPlugin_DSP_la_SOURCES = BeatMatchPlugin.h BeatMatchPlugin.C
BeatMatchPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
BeatMatchPlugin_DSP_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/BeatMatchPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/BeatMatchPlugin_DSP.so"
