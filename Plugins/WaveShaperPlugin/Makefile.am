INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = WaveShaperPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = WaveShaperPlugin.la

WaveShaperPlugin_la_SOURCES =					\
			WaveShaperPluginGUI.h			\
			WaveShaperPluginGUI.C			\
			WaveShaperPlugin.h			\
			WaveShaperPlugin.C
			
WaveShaperPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
WaveShaperPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
