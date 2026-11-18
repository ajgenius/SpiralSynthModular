AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/PianoRollPlugin
plugin_LTLIBRARIES = PianoRollPlugin_DSP.la
PianoRollPlugin_DSP_la_SOURCES = PianoRollPlugin.h ClipTempo.h PianoRollPlugin.C
nodist_PianoRollPlugin_DSP_la_SOURCES = manifest.cpp
PianoRollPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
PianoRollPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/PianoRollPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/PianoRollPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
