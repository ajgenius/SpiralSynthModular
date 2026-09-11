INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = TrigPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = TrigPlugin.la

TrigPlugin_la_SOURCES =					\
			TrigPluginGUI.h			\
			TrigPluginGUI.C			\
			TrigPlugin.h			\
			TrigPlugin.C
			
TrigPlugin_la_LDFLAGS = -shared -module -avoid-version -shared -no-undefined
TrigPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la \
                $(top_builddir)/libspiralcore/libspiralcore.la

# PluginManager dlopens FooPlugin.so.  Drop the libtool .la after install
# (same trick master used) so the directory only contains the module.
install-data-hook:
	-rm -f "$(DESTDIR)$(plugindir)/TrigPlugin.la"
