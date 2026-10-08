AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/OscillatorPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = OscillatorPlugin_DSP.la
OscillatorPlugin_DSP_la_SOURCES = OscillatorPlugin.h OscillatorPlugin.C
OscillatorPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
OscillatorPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/OscillatorPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/OscillatorPlugin_DSP.so"
