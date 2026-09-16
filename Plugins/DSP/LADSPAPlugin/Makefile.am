AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/dsp/LADSPAPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = LADSPAPlugin_DSP.la
LADSPAPlugin_DSP_la_SOURCES = LADSPAInfo.h LADSPAInfo.C LADSPAPlugin.h LADSPAPlugin.C
LADSPAPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
LADSPAPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread $(LRDFLIBS)
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/LADSPAPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/LADSPAPlugin_DSP.so"
