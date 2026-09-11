INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = MoogFilterPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = MoogFilterPlugin.la

MoogFilterPlugin_la_SOURCES =					\
			MoogFilterPluginGUI.h			\
			MoogFilterPluginGUI.C			\
			MoogFilterPlugin.h			\
			MoogFilterPlugin.C
			
MoogFilterPlugin_la_LDFLAGS = -shared -module -avoid-version -shared -no-undefined
MoogFilterPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la \
                $(top_builddir)/libspiralcore/libspiralcore.la

# PluginManager dlopens FooPlugin.so.  Drop the libtool .la after install
# (same trick master used) so the directory only contains the module.
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/MoogFilterPlugin.la"
