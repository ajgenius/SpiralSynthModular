AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/dsp/XFadePlugin
plugin_LTLIBRARIES = XFadePlugin_DSP.la
XFadePlugin_DSP_la_SOURCES = XFadePlugin.h XFadePlugin.C
XFadePlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
XFadePlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/XFadePlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/XFadePlugin_DSP.so"
