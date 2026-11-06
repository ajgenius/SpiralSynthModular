AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/WaveTablePlugin
plugin_LTLIBRARIES = WaveTablePlugin_DSP.la
WaveTablePlugin_DSP_la_SOURCES = WaveTablePlugin.h WaveTablePlugin.C
nodist_WaveTablePlugin_DSP_la_SOURCES = manifest.cpp
WaveTablePlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
WaveTablePlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/WaveTablePlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/WaveTablePlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
