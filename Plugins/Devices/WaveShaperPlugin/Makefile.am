AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/WaveShaperPlugin
plugin_LTLIBRARIES = WaveShaperPlugin_DSP.la
WaveShaperPlugin_DSP_la_SOURCES = WaveShaperPlugin.h WaveShaperPlugin.C
nodist_WaveShaperPlugin_DSP_la_SOURCES = manifest.cpp
WaveShaperPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
WaveShaperPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/WaveShaperPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/WaveShaperPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
