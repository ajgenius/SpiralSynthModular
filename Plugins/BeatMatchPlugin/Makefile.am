INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugin = BeatMatchPlugin.so
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = BeatMatchPlugin.la

BeatMatchPlugin_la_SOURCES =					\
			BeatMatchPluginGUI.h			\
			BeatMatchPluginGUI.C			\
			BeatMatchPlugin.h			\
			BeatMatchPlugin.C
			
BeatMatchPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
BeatMatchPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la

install:$(plugin_LTLIBRARIES)
	cp .libs/$(plugin) $(DESTDIR)/$(plugindir)
