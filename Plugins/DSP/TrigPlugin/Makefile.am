AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/dsp/TrigPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = TrigPlugin_DSP.la
TrigPlugin_DSP_la_SOURCES = TrigPlugin.h TrigPlugin.C
TrigPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
TrigPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/TrigPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/TrigPlugin_DSP.so"
