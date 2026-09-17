AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/dsp/DistributorPlugin
plugin_LTLIBRARIES = DistributorPlugin_DSP.la
DistributorPlugin_DSP_la_SOURCES = DistributorPlugin.h DistributorPlugin.C
DistributorPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
DistributorPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/DistributorPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/DistributorPlugin_DSP.so"
