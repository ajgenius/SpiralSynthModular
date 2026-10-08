AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/TransposePlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = TransposePlugin_DSP.la
TransposePlugin_DSP_la_SOURCES = TransposePlugin.h TransposePlugin.C
TransposePlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
TransposePlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/TransposePlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/TransposePlugin_DSP.so"
