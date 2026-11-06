AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/SmoothPlugin
plugin_LTLIBRARIES = SmoothPlugin_DSP.la
SmoothPlugin_DSP_la_SOURCES = SmoothPlugin.h SmoothPlugin.C
nodist_SmoothPlugin_DSP_la_SOURCES = manifest.cpp
SmoothPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SmoothPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SmoothPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SmoothPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
