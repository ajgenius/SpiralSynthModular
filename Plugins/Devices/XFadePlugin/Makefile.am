AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/XFadePlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = XFadePlugin_DSP.la
XFadePlugin_DSP_la_SOURCES = XFadePlugin.h XFadePlugin.C
XFadePlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
XFadePlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/XFadePlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/XFadePlugin_DSP.so"
