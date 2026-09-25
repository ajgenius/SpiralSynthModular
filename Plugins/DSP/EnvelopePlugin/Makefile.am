AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/EnvelopePlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = EnvelopePlugin_DSP.la
EnvelopePlugin_DSP_la_SOURCES = EnvelopePlugin.h EnvelopePlugin.C
EnvelopePlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
EnvelopePlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/EnvelopePlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/EnvelopePlugin_DSP.so"
