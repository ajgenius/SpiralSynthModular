INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = OutputPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = OutputPlugin.la

OutputPlugin_la_SOURCES =					\
			OutputPluginGUI.h			\
			OutputPluginGUI.C			\
			OutputPlugin.h			\
			OutputPlugin.C
			
OutputPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
OutputPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
