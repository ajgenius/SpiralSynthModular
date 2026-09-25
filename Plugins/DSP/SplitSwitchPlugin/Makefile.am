AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = @PLUGINDIR@/dsp/SplitSwitchPlugin
dist_plugin_DATA = info.json
plugin_LTLIBRARIES = SplitSwitchPlugin_DSP.la
SplitSwitchPlugin_DSP_la_SOURCES = SplitSwitchPlugin.h SplitSwitchPlugin.C
SplitSwitchPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SplitSwitchPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SplitSwitchPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SplitSwitchPlugin_DSP.so"
