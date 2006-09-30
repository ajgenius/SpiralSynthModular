INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = RingModPlugin.la
RingModPlugin_la_SOURCES =					\
			RingModPluginGUI.h			\
			RingModPluginGUI.C			\
			RingModPlugin.h			\
			RingModPlugin.C
			
RingModPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
RingModPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la
