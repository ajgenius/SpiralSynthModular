AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/dsp/OutputPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = OutputPlugin_DSP.la
OutputPlugin_DSP_la_SOURCES = OutputPlugin.h OutputPlugin.C OutputAudioClient.C OutputAudioClient.h
OutputPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
OutputPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/OutputPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/OutputPlugin_DSP.so"
