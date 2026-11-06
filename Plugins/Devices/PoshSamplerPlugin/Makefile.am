AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/PoshSamplerPlugin
plugin_LTLIBRARIES = PoshSamplerPlugin_DSP.la
PoshSamplerPlugin_DSP_la_SOURCES = PoshSamplerPlugin.h PoshSamplerPlugin.C
nodist_PoshSamplerPlugin_DSP_la_SOURCES = manifest.cpp
PoshSamplerPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
PoshSamplerPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/PoshSamplerPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/PoshSamplerPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
