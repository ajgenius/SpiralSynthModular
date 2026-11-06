AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/OperatorPlugin
plugin_LTLIBRARIES = OperatorPlugin_DSP.la
OperatorPlugin_DSP_la_SOURCES = OperatorPlugin.h OperatorPlugin.C
nodist_OperatorPlugin_DSP_la_SOURCES = manifest.cpp
OperatorPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
OperatorPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/OperatorPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/OperatorPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
