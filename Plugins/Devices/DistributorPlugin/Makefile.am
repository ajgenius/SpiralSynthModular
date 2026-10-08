AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/DistributorPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = DistributorPlugin_DSP.la
DistributorPlugin_DSP_la_SOURCES = DistributorPlugin.h DistributorPlugin.C
DistributorPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
DistributorPlugin_DSP_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/DistributorPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/DistributorPlugin_DSP.so"
