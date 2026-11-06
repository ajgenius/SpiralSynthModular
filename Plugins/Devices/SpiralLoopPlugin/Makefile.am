AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/SpiralLoopPlugin
plugin_LTLIBRARIES = SpiralLoopPlugin_DSP.la
SpiralLoopPlugin_DSP_la_SOURCES = SpiralLoopPlugin.h SpiralLoopPlugin.C
nodist_SpiralLoopPlugin_DSP_la_SOURCES = manifest.cpp
SpiralLoopPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SpiralLoopPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SpiralLoopPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SpiralLoopPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
