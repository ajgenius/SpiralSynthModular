AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/SmoothPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = SmoothPlugin_DSP.la
SmoothPlugin_DSP_la_SOURCES = SmoothPlugin.h SmoothPlugin.C
SmoothPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SmoothPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SmoothPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SmoothPlugin_DSP.so"
