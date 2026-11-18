AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/ClockPlugin
plugin_LTLIBRARIES = ClockPlugin_DSP.la
ClockPlugin_DSP_la_SOURCES = ClockPlugin.h MusicalClock.h ClockPlugin.C
nodist_ClockPlugin_DSP_la_SOURCES = manifest.cpp
ClockPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
ClockPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/ClockPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/ClockPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
