AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/dsp/DelayPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = DelayPlugin_DSP.la
DelayPlugin_DSP_la_SOURCES = DelayPlugin.h DelayPlugin.C
DelayPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
DelayPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/DelayPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/DelayPlugin_DSP.so"
