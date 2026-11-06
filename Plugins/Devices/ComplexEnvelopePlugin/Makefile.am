AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/ComplexEnvelopePlugin
plugin_LTLIBRARIES = ComplexEnvelopePlugin_DSP.la
ComplexEnvelopePlugin_DSP_la_SOURCES = ComplexEnvelopePlugin.h ComplexEnvelopePlugin.C
nodist_ComplexEnvelopePlugin_DSP_la_SOURCES = manifest.cpp
ComplexEnvelopePlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
ComplexEnvelopePlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/ComplexEnvelopePlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/ComplexEnvelopePlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
