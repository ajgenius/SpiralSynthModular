AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = SwitchPlugin_DSP.la
SwitchPlugin_DSP_la_SOURCES = SwitchPlugin.h SwitchPlugin.C
SwitchPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SwitchPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SwitchPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SwitchPlugin_DSP.so"
