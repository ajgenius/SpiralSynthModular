AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/EnvFollowerPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = EnvFollowerPlugin_DSP.la
EnvFollowerPlugin_DSP_la_SOURCES = EnvFollowerPlugin.h EnvFollowerPlugin.C
EnvFollowerPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
EnvFollowerPlugin_DSP_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/EnvFollowerPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/EnvFollowerPlugin_DSP.so"
