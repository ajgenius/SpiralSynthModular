AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = PoshSamplerPlugin_DSP.la
PoshSamplerPlugin_DSP_la_SOURCES = PoshSamplerPlugin.h PoshSamplerPlugin.C
PoshSamplerPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
PoshSamplerPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/PoshSamplerPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/PoshSamplerPlugin_DSP.so"
