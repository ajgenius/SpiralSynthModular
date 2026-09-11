AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = StereoMixerPlugin_DSP.la
StereoMixerPlugin_DSP_la_SOURCES = StereoMixerPlugin.h StereoMixerPlugin.C
StereoMixerPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
StereoMixerPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/StereoMixerPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/StereoMixerPlugin_DSP.so"
