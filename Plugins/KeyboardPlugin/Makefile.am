INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm

plugin = KeyboardPlugin.so	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = KeyboardPlugin.la
KeyboardPlugin_la_SOURCES =					\
			KeyboardPluginGUI.h			\
			KeyboardPluginGUI.C			\
			KeyboardPlugin.h			\
			KeyboardPlugin.C
			
KeyboardPlugin_la_LDFLAGS = -shared -module -avoid-version -shared -no-undefined
KeyboardPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la \
                $(top_builddir)/libspiralcore/libspiralcore.la

# PluginManager dlopens FooPlugin.so.  Drop the libtool .la after install
# (same trick master used) so the directory only contains the module.
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/KeyboardPlugin.la"
