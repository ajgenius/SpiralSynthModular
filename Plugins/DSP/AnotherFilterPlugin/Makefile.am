AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/AnotherFilterPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = AnotherFilterPlugin_DSP.la
AnotherFilterPlugin_DSP_la_SOURCES = AnotherFilterPlugin.h AnotherFilterPlugin.C
AnotherFilterPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
AnotherFilterPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/AnotherFilterPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/AnotherFilterPlugin_DSP.so"
