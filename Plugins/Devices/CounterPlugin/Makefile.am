AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/CounterPlugin
plugin_LTLIBRARIES = CounterPlugin_DSP.la
CounterPlugin_DSP_la_SOURCES = CounterPlugin.h CounterPlugin.C
nodist_CounterPlugin_DSP_la_SOURCES = manifest.cpp
CounterPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
CounterPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/CounterPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/CounterPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
