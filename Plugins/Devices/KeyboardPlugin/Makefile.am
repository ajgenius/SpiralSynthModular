AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/KeyboardPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = KeyboardPlugin_DSP.la
KeyboardPlugin_DSP_la_SOURCES = KeyboardPlugin.h KeyboardPlugin.C
KeyboardPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
KeyboardPlugin_DSP_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/KeyboardPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/KeyboardPlugin_DSP.so"
