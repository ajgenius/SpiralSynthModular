AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/EchoPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = EchoPlugin_DSP.la
EchoPlugin_DSP_la_SOURCES = EchoPlugin.h EchoPlugin.C
EchoPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
EchoPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/EchoPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/EchoPlugin_DSP.so"
