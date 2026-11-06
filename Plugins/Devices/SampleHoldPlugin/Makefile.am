AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/SampleHoldPlugin
plugin_LTLIBRARIES = SampleHoldPlugin_DSP.la
SampleHoldPlugin_DSP_la_SOURCES = SampleHoldPlugin.h SampleHoldPlugin.C
nodist_SampleHoldPlugin_DSP_la_SOURCES = manifest.cpp
SampleHoldPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SampleHoldPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SampleHoldPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SampleHoldPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
