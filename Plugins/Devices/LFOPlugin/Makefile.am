AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/LFOPlugin
plugin_LTLIBRARIES = LFOPlugin_DSP.la
LFOPlugin_DSP_la_SOURCES = LFOPlugin.h LFOPlugin.C
nodist_LFOPlugin_DSP_la_SOURCES = manifest.cpp
LFOPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
LFOPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/LFOPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/LFOPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
