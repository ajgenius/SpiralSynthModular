AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/dsp/SpiralLoopPlugin
plugin_LTLIBRARIES = SpiralLoopPlugin_DSP.la
SpiralLoopPlugin_DSP_la_SOURCES = SpiralLoopPlugin.h SpiralLoopPlugin.C
SpiralLoopPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SpiralLoopPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SpiralLoopPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SpiralLoopPlugin_DSP.so"
