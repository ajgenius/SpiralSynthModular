INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = SVFilterPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = SVFilterPlugin.la

SVFilterPlugin_la_SOURCES =					\
			SVFilterPluginGUI.h			\
			SVFilterPluginGUI.C			\
			SVFilterPlugin.h			\
			SVFilterPlugin.C
			
SVFilterPlugin_la_LDFLAGS = -shared -module -avoid-version -shared -no-undefined
SVFilterPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la \
                $(top_builddir)/libspiralcore/libspiralcore.la

# PluginManager dlopens FooPlugin.so.  Drop the libtool .la after install
# (same trick master used) so the directory only contains the module.
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/SVFilterPlugin.la"
