AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/LADSPAPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = LADSPAPlugin_DSP.la
LADSPAPlugin_DSP_la_SOURCES = LADSPAInfo.h LADSPAInfo.C LADSPAPlugin.h LADSPAPlugin.C
LADSPAPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
LADSPAPlugin_DSP_la_LIBADD = $(top_builddir)/libspiralcore/libspiralcore.la -lm $(LRDFLIBS)
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/LADSPAPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/LADSPAPlugin_DSP.so"
