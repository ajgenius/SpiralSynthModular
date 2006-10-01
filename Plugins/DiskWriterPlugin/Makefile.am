INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = DiskWriterPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = DiskWriterPlugin.la

DiskWriterPlugin_la_SOURCES =					\
			DiskWriterPluginGUI.h			\
			DiskWriterPluginGUI.C			\
			DiskWriterPlugin.h			\
			DiskWriterPlugin.C
			
DiskWriterPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
DiskWriterPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
