INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = WaveTablePlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = WaveTablePlugin.la

WaveTablePlugin_la_SOURCES =					\
			WaveTablePluginGUI.h			\
			WaveTablePluginGUI.C			\
			WaveTablePlugin.h			\
			WaveTablePlugin.C
			
WaveTablePlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
WaveTablePlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la \
                $(top_builddir)/libspiralcore/libspiralcore.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
