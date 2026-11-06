AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/MatrixPlugin
plugin_LTLIBRARIES = MatrixPlugin_DSP.la
MatrixPlugin_DSP_la_SOURCES = MatrixPlugin.h MatrixPlugin.C
nodist_MatrixPlugin_DSP_la_SOURCES = manifest.cpp
MatrixPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
MatrixPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/MatrixPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/MatrixPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
