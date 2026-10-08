AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/JackPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = JackPlugin_DSP.la
JackPlugin_DSP_la_SOURCES = JackPlugin.h JackPlugin.C
JackPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
JackPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/JackPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/JackPlugin_DSP.so"
