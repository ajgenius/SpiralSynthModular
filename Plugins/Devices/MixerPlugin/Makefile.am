AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/MixerPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = MixerPlugin_DSP.la
MixerPlugin_DSP_la_SOURCES = MixerPlugin.h MixerPlugin.C
MixerPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
MixerPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/MixerPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/MixerPlugin_DSP.so"
