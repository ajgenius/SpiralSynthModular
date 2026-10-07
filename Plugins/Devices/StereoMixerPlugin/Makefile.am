AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/StereoMixerPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = StereoMixerPlugin_DSP.la
StereoMixerPlugin_DSP_la_SOURCES = StereoMixerPlugin.h StereoMixerPlugin.C
StereoMixerPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
StereoMixerPlugin_DSP_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/StereoMixerPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/StereoMixerPlugin_DSP.so"
