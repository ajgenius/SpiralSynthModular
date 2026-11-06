AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/ControllerPlugin
plugin_LTLIBRARIES = ControllerPlugin_DSP.la
ControllerPlugin_DSP_la_SOURCES = ControllerPlugin.h ControllerPlugin.C
nodist_ControllerPlugin_DSP_la_SOURCES = manifest.cpp
ControllerPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
ControllerPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/ControllerPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/ControllerPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
