AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/FormantFilterPlugin
plugin_LTLIBRARIES = FormantFilterPlugin_DSP.la
FormantFilterPlugin_DSP_la_SOURCES = FormantFilterPlugin.h FormantFilterPlugin.C
nodist_FormantFilterPlugin_DSP_la_SOURCES = manifest.cpp
FormantFilterPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
FormantFilterPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/FormantFilterPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/FormantFilterPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
