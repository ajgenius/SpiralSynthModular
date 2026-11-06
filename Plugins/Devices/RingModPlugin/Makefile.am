AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/RingModPlugin
plugin_LTLIBRARIES = RingModPlugin_DSP.la
RingModPlugin_DSP_la_SOURCES = RingModPlugin.h RingModPlugin.C
nodist_RingModPlugin_DSP_la_SOURCES = manifest.cpp
RingModPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
RingModPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/RingModPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/RingModPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
