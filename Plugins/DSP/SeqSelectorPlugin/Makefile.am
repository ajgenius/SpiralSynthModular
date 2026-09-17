AM_CPPFLAGS = -I$(top_srcdir)/GUI/Widgets -I$(top_srcdir)/libspiralcore/include -I$(top_srcdir)
EXTRA_DIST = SpiralIcon.xpm
plugindir = $(libdir)/SpiralPlugins/$(PACKAGE_VERSION)/dsp/SeqSelectorPlugin
plugin_LTLIBRARIES = SeqSelectorPlugin_DSP.la
SeqSelectorPlugin_DSP_la_SOURCES = SeqSelectorPlugin.h SeqSelectorPlugin.C
SeqSelectorPlugin_DSP_la_LDFLAGS = -module -avoid-version -shared -no-undefined
SeqSelectorPlugin_DSP_la_LIBADD = $(top_builddir)/GUI/Widgets/libSpiralPlugin.la $(top_builddir)/libspiralcore/libspiralcore.la -lm -ldl -lpthread
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SeqSelectorPlugin_DSP.la"
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/SeqSelectorPlugin_DSP.so"
