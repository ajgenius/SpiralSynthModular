AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/SwitchPlugin
plugin_LTLIBRARIES = SwitchPlugin_DSP.la
SwitchPlugin_DSP_la_SOURCES = SwitchPlugin.h SwitchPlugin.C
nodist_SwitchPlugin_DSP_la_SOURCES = manifest.cpp
SwitchPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SwitchPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SwitchPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SwitchPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
