AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/MasherPlugin
plugin_LTLIBRARIES = MasherPlugin_DSP.la
MasherPlugin_DSP_la_SOURCES = MasherPlugin.h MasherPlugin.C
nodist_MasherPlugin_DSP_la_SOURCES = manifest.cpp
MasherPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
MasherPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/MasherPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/MasherPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
