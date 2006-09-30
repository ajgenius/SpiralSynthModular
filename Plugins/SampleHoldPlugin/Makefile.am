INCLUDES = 						\
		$(FLTK_CXXFLAGS)			\
		$(FLTK_CFLAGS)				\
		-I$(top_builddir)/GUI/Widgets		\
		-I$(top_builddir)/SpiralSound
		
EXTRA_DIST = SpiralIcon.xpm
	
plugindir = $(libdir)/SpiralPlugins
plugin_LTLIBRARIES = SampleHoldPlugin.la
SampleHoldPlugin_la_SOURCES =					\
			SampleHoldPluginGUI.h			\
			SampleHoldPluginGUI.C			\
			SampleHoldPlugin.h			\
			SampleHoldPlugin.C
			
SampleHoldPlugin_la_LDFLAGS = -shared -module -avoid-version -no-undefined
SampleHoldPlugin_la_LIBADD = 							\
			$(FLTK_LIBS)					\
			-lm -ldl -lpthread				\
			$(top_builddir)/GUI/Widgets/libSpiralWidgets.la	\
			$(top_builddir)/SpiralSound/libSpiralCommon.la
