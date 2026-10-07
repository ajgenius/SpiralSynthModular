AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/SampleHoldPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = SampleHoldPlugin_DSP.la
SampleHoldPlugin_DSP_la_SOURCES = SampleHoldPlugin.h SampleHoldPlugin.C
SampleHoldPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SampleHoldPlugin_DSP_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SampleHoldPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SampleHoldPlugin_DSP.so"
