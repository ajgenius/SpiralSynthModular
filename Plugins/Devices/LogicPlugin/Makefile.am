AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/LogicPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = LogicPlugin_DSP.la
LogicPlugin_DSP_la_SOURCES = LogicPlugin.h LogicPlugin.C
LogicPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
LogicPlugin_DSP_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/LogicPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/LogicPlugin_DSP.so"
