AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/dsp/FilterPlugin
plugin_LTLIBRARIES = FilterPlugin_DSP.la
FilterPlugin_DSP_la_SOURCES = iir_filter.h iir_filter.C FilterPlugin.h FilterPlugin.C
FilterPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
FilterPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/FilterPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/FilterPlugin_DSP.so"
