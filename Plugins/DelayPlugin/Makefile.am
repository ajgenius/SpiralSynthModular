INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = DelayPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = DelayPlugin.la

DelayPlugin_la_SOURCES =					\
			DelayPluginGUI.h			\
			DelayPluginGUI.C			\
			DelayPlugin.h			\
			DelayPlugin.C
			
DelayPlugin_la_LDFLAGS = -shared -module -avoid-version -shared -no-undefined
DelayPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la \
                $(top_builddir)/libspiralcore/libspiralcore.la

# PluginManager dlopens FooPlugin.so.  Drop the libtool .la after install
# (same trick master used) so the directory only contains the module.
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/DelayPlugin.la"
