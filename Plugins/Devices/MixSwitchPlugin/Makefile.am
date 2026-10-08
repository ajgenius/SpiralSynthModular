AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/MixSwitchPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = MixSwitchPlugin_DSP.la
MixSwitchPlugin_DSP_la_SOURCES = MixSwitchPlugin.h MixSwitchPlugin.C
MixSwitchPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
MixSwitchPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/MixSwitchPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/MixSwitchPlugin_DSP.so"
