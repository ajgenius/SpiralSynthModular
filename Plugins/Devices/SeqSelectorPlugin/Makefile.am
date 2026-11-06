AM_CPPFLAGS = $(SPIRALGUI_CFLAGS) $(SPIRALCORE_CFLAGS) -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/SeqSelectorPlugin
plugin_LTLIBRARIES = SeqSelectorPlugin_DSP.la
SeqSelectorPlugin_DSP_la_SOURCES = SeqSelectorPlugin.h SeqSelectorPlugin.C
nodist_SeqSelectorPlugin_DSP_la_SOURCES = manifest.cpp
SeqSelectorPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SeqSelectorPlugin_DSP_la_LIBADD = $(top_builddir)/libs/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SeqSelectorPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SeqSelectorPlugin_DSP.so"

include $(top_srcdir)/Plugins/Devices/manifest.am
