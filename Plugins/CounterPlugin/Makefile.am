INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = CounterPlugin.la
CounterPlugin_la_SOURCES =					\
			CounterPluginGUI.h			\
			CounterPluginGUI.C			\
			CounterPlugin.h			\
			CounterPlugin.C
			
CounterPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
CounterPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la
