AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/LFOPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = LFOPlugin_DSP.la
LFOPlugin_DSP_la_SOURCES = LFOPlugin.h LFOPlugin.C
LFOPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
LFOPlugin_DSP_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/LFOPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/LFOPlugin_DSP.so"
