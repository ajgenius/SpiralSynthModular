INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_srcdir)/libspiralcore/include
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = EnvelopePlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = EnvelopePlugin.la

EnvelopePlugin_la_SOURCES =					\
			../../GUI/EnvelopePlugin/EnvelopePluginGUI.h			\
			../../GUI/EnvelopePlugin/EnvelopePluginGUI.C			\
			EnvelopePlugin.h			\
			EnvelopePlugin.C
			
EnvelopePlugin_la_LDFLAGS = -shared -module -avoid-version -shared -no-undefined
EnvelopePlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
                $(top_builddir)/libspiralcore/libspiralcore.la

# PluginManager dlopens FooPlugin.so.  Drop the libtool .la after install
# (same trick master used) so the directory only contains the module.
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/EnvelopePlugin.la"

# The installed .la is removed, so libtool cannot discover the module at uninstall.
uninstall-local:
	rm -f "$(DESTDIR)$(plugindir)/$(plugin)"

AM_CPPFLAGS = -I$(srcdir)

AM_CPPFLAGS += -I$(srcdir) -I$(top_srcdir)/Plugins/GUI/EnvelopePlugin
