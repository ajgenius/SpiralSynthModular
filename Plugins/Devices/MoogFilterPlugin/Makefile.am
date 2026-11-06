AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/MoogFilterPlugin
plugin_LTLIBRARIES = MoogFilterPlugin_DSP.la
MoogFilterPlugin_DSP_la_SOURCES = MoogFilterPlugin.h MoogFilterPlugin.C
nodist_MoogFilterPlugin_DSP_la_SOURCES = manifest.cpp
MoogFilterPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
MoogFilterPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/MoogFilterPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/MoogFilterPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
