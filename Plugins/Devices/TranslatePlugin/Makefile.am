AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/TranslatePlugin
plugin_LTLIBRARIES = TranslatePlugin_DSP.la
TranslatePlugin_DSP_la_SOURCES = TranslatePlugin.h TranslatePlugin.C
nodist_TranslatePlugin_DSP_la_SOURCES = manifest.cpp
TranslatePlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
TranslatePlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/TranslatePlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/TranslatePlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
