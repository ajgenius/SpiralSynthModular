AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = JackPlugin_DSP.la
JackPlugin_DSP_la_SOURCES = JackPlugin.h JackPlugin.C
JackPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
JackPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread -ljack
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/JackPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/JackPlugin_DSP.so"
