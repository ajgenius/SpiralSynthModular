INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = FlipflopPlugin.la
FlipflopPlugin_la_SOURCES =					\
			FlipflopPluginGUI.h			\
			FlipflopPluginGUI.C			\
			FlipflopPlugin.h			\
			FlipflopPlugin.C
			
FlipflopPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
FlipflopPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la
